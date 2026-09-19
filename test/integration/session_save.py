#!/usr/bin/env python3
"""Linux regression test for overlapping session saves.

Run with Python 3 and strace 6.0 or newer:
    python3 test/integration/session_save.py --binary src/rtorrent

Only synthetic, stopped torrents are loaded. The test delays the sixth openat
in each thread, which is the initial full-save worker's reopen of the rtorrent
sidecar. The kernel still supplies every syscall result. Traces must confirm
that the intended file open was delayed; unsupported tracing fails the test.
"""

import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import unittest
import xmlrpc.client


def bencode(value):
    if isinstance(value, int):
        return b'i' + str(value).encode() + b'e'
    if isinstance(value, bytes):
        return str(len(value)).encode() + b':' + value
    return b'd' + b''.join(bencode(k) + bencode(v) for k, v in sorted(value.items())) + b'e'


def wait_for(predicate, message, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.01)
    raise AssertionError(message)


class SessionSaveTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='rt-save-')
        self.root = Path(self.directory.name)
        self.session = self.root / 'session'
        self.session.mkdir()
        (self.root / 'data').mkdir()
        self.process = None
        self.pid = None
        self.boot = 0
        self.payloads = {}
        self.output = None
        self.addCleanup(self.cleanup)
        self.start(delay=True)

    def cleanup(self):
        if self.process is not None and self.process.poll() is None:
            os.killpg(self.process.pid, signal.SIGKILL)
            self.process.wait(timeout=10)
        if self.output is not None:
            self.output.close()
        if ARGS.artifacts:
            shutil.copytree(self.root, Path(ARGS.artifacts) / self._testMethodName,
                            ignore=shutil.ignore_patterns('rpc.sock'))
        self.directory.cleanup()

    def rpc(self, method, *params):
        body = xmlrpc.client.dumps(params, methodname=method).encode()
        headers = (b'CONTENT_LENGTH\0' + str(len(body)).encode() +
                   b'\0SCGI\x001\0REQUEST_METHOD\0POST\0REQUEST_URI\0/RPC2\0')
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
            connection.settimeout(10)
            connection.connect(str(self.root / 'rpc.sock'))
            connection.sendall(str(len(headers)).encode() + b':' + headers + b',' + body)
            response = b''
            while True:
                part = connection.recv(65536)
                if not part:
                    break
                response += part
        return xmlrpc.client.loads(response.split(b'\r\n\r\n', 1)[1])[0][0]

    def start(self, delay):
        self.boot += 1
        self.pid = None
        (self.root / 'rpc.sock').unlink(missing_ok=True)
        config = self.root / 'rtorrent.rc'
        config.write_text(
            f'session.path.set = {self.session}\n'
            f'directory.default.set = {self.root}/data\n'
            f'network.scgi.open_local = {self.root}/rpc.sock\n'
            'network.port_open.set = no\n'
            'dht.mode.set = disable\n'
            f'log.open_file = "test", "{self.root}/engine-{self.boot}.log"\n'
            'log.add_output = "info", "test"\n'
            'log.add_output = "session_events", "test"\n')
        trace = [ARGS.strace, '-f', '-ttt', '-s', '256', '-o', str(self.root / f'trace-{self.boot}.log'),
                 '-e', 'trace=openat,close,fdatasync,fsync,rename']
        if delay:
            trace += ['-e', 'inject=openat:delay_enter=3s:when=6']
        self.output = (self.root / f'output-{self.boot}.log').open('wb')
        self.process = subprocess.Popen(
            trace + [ARGS.binary, '-D', '-n', '-o', 'import=' + str(config), '-o', 'system.daemon.set=true'],
            stdout=self.output, stderr=subprocess.STDOUT,
            env=dict(os.environ, TERM='xterm'), start_new_session=True)

        def ready():
            if self.process.poll() is not None:
                raise AssertionError((self.root / f'output-{self.boot}.log').read_text())
            try:
                self.pid = self.rpc('system.pid')
                return True
            except (OSError, xmlrpc.client.Error):
                return False
        wait_for(ready, 'rTorrent did not start')

    def stop(self):
        os.kill(self.pid, signal.SIGINT)
        self.assertEqual(self.process.wait(timeout=20), 0)
        self.output.close()
        self.output = None

    def add(self, name):
        payload = (name + '\n').encode() * 100
        self.payloads[name] = payload
        (self.root / 'data' / name).write_bytes(payload)
        info = {b'name': name.encode(), b'length': len(payload), b'piece length': 16384,
                b'pieces': hashlib.sha1(payload).digest(), b'private': 1}
        torrent = self.root / (name + '.torrent')
        torrent.write_bytes(bencode({b'announce': b'http://127.0.0.1:9/announce', b'info': info}))
        torrent_hash = hashlib.sha1(bencode(info)).hexdigest().upper()
        self.rpc('load.normal', '', str(torrent))
        wait_for(lambda: torrent_hash in self.rpc('download_list'), 'Torrent was not loaded')
        return torrent_hash

    def wait_for_blocked_save(self, torrent_hash):
        trace = self.root / 'trace-1.log'
        sidecar = str(self.session / (torrent_hash + '.torrent.rtorrent.new'))
        wait_for(lambda: f'"{sidecar}", O_WRONLY <unfinished ...>' in trace.read_text(),
                 'Full-save worker did not reach its delayed reopen')
        self.assertFalse((self.session / (torrent_hash + '.torrent')).exists())

    def verify_restart(self, expected):
        self.stop()
        trace = (self.root / 'trace-1.log').read_text()
        # Match the worker's unfinished open with its delayed completion.
        for torrent_hash in expected:
            path = re.escape(str(self.session / (torrent_hash + '.torrent.rtorrent.new')))
            match = re.search(r'^(\d+)\s+[^\n]*openat\([^\n]*"' + path +
                              r'", O_WRONLY <unfinished', trace, re.M)
            self.assertIsNotNone(match, 'Missing delayed session open in trace')
            self.assertRegex(trace, r'(?m)^' + match.group(1) +
                             r'\s+[^\n]*<\.\.\. openat resumed>[^\n]*\(DELAYED\)')
        self.assertNotIn('Storage errors saving session data', (self.root / 'engine-1.log').read_text())
        self.assertFalse(list(self.session.glob('*.new')), 'Uncommitted session files remain')
        self.start(delay=False)
        self.assertCountEqual(self.rpc('download_list'), expected)
        for torrent_hash, label in expected.items():
            self.assertEqual(self.rpc('d.custom1', torrent_hash), label)
        for name, payload in self.payloads.items():
            self.assertEqual((self.root / 'data' / name).read_bytes(), payload)
        self.stop()

    def test_resume_waits_for_initial_full_save(self):
        torrent_hash = self.add('resume.bin')
        self.wait_for_blocked_save(torrent_hash)
        self.rpc('d.custom1.set', torrent_hash, 'first')
        self.rpc('d.save_resume', torrent_hash)
        self.rpc('d.custom1.set', torrent_hash, 'latest')
        self.rpc('session.save')
        time.sleep(4)
        self.verify_restart({torrent_hash: 'latest'})

    def test_full_and_resume_requests_keep_latest_fields(self):
        torrent_hash = self.add('full.bin')
        self.wait_for_blocked_save(torrent_hash)
        self.rpc('d.custom1.set', torrent_hash, 'full')
        self.rpc('d.save_full_session', torrent_hash)
        self.rpc('d.custom1.set', torrent_hash, 'latest')
        self.rpc('session.save')
        time.sleep(7)
        self.verify_restart({torrent_hash: 'latest'})

    def test_other_torrent_can_save_while_first_is_blocked(self):
        first = self.add('first.bin')
        self.wait_for_blocked_save(first)
        self.rpc('d.custom1.set', first, 'first')
        self.rpc('d.save_resume', first)
        second = self.add('second.bin')
        self.wait_for_blocked_save(second)
        self.assertFalse((self.session / (first + '.torrent')).exists(),
                         'Second torrent waited for the first full save')
        self.rpc('d.custom1.set', second, 'second')
        self.rpc('session.save')
        time.sleep(4)
        self.verify_restart({first: 'first', second: 'second'})

    def test_shutdown_waits_for_conflicting_save(self):
        torrent_hash = self.add('shutdown.bin')
        self.wait_for_blocked_save(torrent_hash)
        self.rpc('d.custom1.set', torrent_hash, 'shutdown')
        self.rpc('session.save')
        self.verify_restart({torrent_hash: 'shutdown'})
        log = (self.root / 'engine-1.log').read_text()
        self.assertIn('flushing all pending saves', log)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=lambda path: str(Path(path).resolve()))
    parser.add_argument('--strace', default=shutil.which('strace'))
    parser.add_argument('--artifacts', help='Copy each test directory here before cleanup')
    ARGS = parser.parse_args()
    if not ARGS.strace:
        parser.error('strace is required; regression coverage cannot be skipped')
    if ARGS.artifacts:
        Path(ARGS.artifacts).mkdir(parents=True, exist_ok=False)
    unittest.main(argv=[__file__], verbosity=2)
