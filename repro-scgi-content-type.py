#!/usr/bin/env python3
"""Reproduce SCGI content-type auto-detection bug in rtorrent.

Sends a JSON SCGI request without Content-Type header, splitting the
TCP stream so that the header arrives before the body.  If the server
peeks at the body start (m_buffer[m_body]) before the body has arrived,
it reads a null byte instead of '{' and defaults to XML — causing the
JSON-RPC call to fail.

Usage:
    python3 repro-scgi-content-type.py host:port
    python3 repro-scgi-content-type.py 127.0.0.1:5101
"""

import socket
import sys
import time


SCGI_HEADERS = (
    b"CONTENT_LENGTH\x00154\x00"
    b"SCGI\x001\x00"
)

JSON_BODY = (
    b'{"method":"system.listMethods","params":[],"id":1}'
)

HEADER_LENGTH = len(SCGI_HEADERS)
HEADER_LEN_STR = str(HEADER_LENGTH).encode()


def send_in_chunks(host: str, port: int, chunks: list[bytes]) -> bytes:
    with socket.create_connection((host, port), timeout=5) as sock:
        for data in chunks:
            sock.sendall(data)
            time.sleep(0.05)
        return sock.recv(4096)


def test_normal(host: str, port: int) -> bool:
    """All in one segment (works either way)."""
    full_request = HEADER_LEN_STR + b":" + SCGI_HEADERS + b"," + JSON_BODY
    response = send_in_chunks(host, port, [full_request])
    ok = len(response) > 0
    print(f"  Normal send: {'OK' if ok else 'FAIL'} (got {len(response)} bytes)")
    return ok


def test_split_header_body(host: str, port: int) -> bool:
    """Header arrives first, body second (triggers the bug)."""
    chunks = [
        HEADER_LEN_STR + b":" + SCGI_HEADERS + b",",
        JSON_BODY,
    ]
    response = send_in_chunks(host, port, chunks)
    ok = len(response) > 0
    print(f"  Split header/body: {'OK' if ok else 'FAIL'} (got {len(response)} bytes)")
    return ok


def main() -> None:
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)

    addr = sys.argv[1]
    if ":" in addr:
        host, port_str = addr.rsplit(":", 1)
        port = int(port_str)
    else:
        host, port = addr, 80

    print(f"=== Reproducing SCGI content-type defer bug against {host}:{port} ===\n")

    normal_ok = test_normal(host, port)
    if not normal_ok:
        print("\nServer doesn't seem to be running or reachable. Aborting.")
        sys.exit(1)

    split_ok = test_split_header_body(host, port)

    print()
    if not split_ok:
        print("RESULT: BUG CONFIRMED — JSON request fails when body arrives after header.")
        print("         detect_content_type() peeked at m_buffer[m_body] which was \\0 (not '{').")
    else:
        print("RESULT: Bug not reproduced — server handles content-type defer correctly.")
    print(f"  normal={normal_ok} split_header_body={split_ok}")


if __name__ == "__main__":
    main()
