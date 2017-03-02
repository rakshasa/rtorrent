# rTorrent Configuration Template

## Overview

This page contains a modern rTorrent configuration that provides a good starting point.
Its expressed purpose is to replace that years-old rotting piece of garbage that people still use to create their first configuration. Friends don't let friends use that!

It uses `0.9.x` syntax and is tested using `0.9.6`, so be sure to run snippets you add through
the migration script. Be considerate in what you add, this is supposed
to help new users to jump-start their installation, so keep things
out that are not applicable to a wide range of people. Place advanced
use-cases in the appropriate sections of the wiki, like the
[Configuration Guide](https://github.com/rakshasa/rtorrent/wiki/Config-Guide).


## Using the Template

To create the directory tree this configuration expects, use these commands:

```sh
basedir="$HOME/rtorrent"
mkdir -p "$basedir"/{.session,download,log,watch/load,watch/start}
```

The `basedir` value has to be the same you set in `cfg.basedir` below.

Use this command to get a copy of the template to your disk:

```sh
curl -s https://raw.githubusercontent.com/wiki/rakshasa/rtorrent/CONFIG-Template.md \
    | grep -A9999 '^######' | grep -B9999 '^### END' >~/.rtorrent.rc
```

## The Template

:construction: | This is NOT complete yet!
---: | :---

```ini
#############################################################################
# A minimal rTorrent configuration that provides the basic features
# you want to have in addition to the built-in defaults.
#
# See https://github.com/rakshasa/rtorrent/wiki/CONFIG-Template
# for an up-to-date version.
#############################################################################

# Instance layout (base paths)
method.insert = cfg.basedir,    private|const|string, (cat,"/home/USERNAME/rtorrent/")
method.insert = cfg.watch,      private|const|string, (cat,(cfg.basedir),"watch/")
method.insert = cfg.logs,       private|const|string, (cat,(cfg.basedir),"log/")
method.insert = cfg.logfile,    private|const|string, (cat,(cfg.logs),"rtorrent-",(system.time),".log")

# Listening port for incoming peer traffic (fixed; you can also randomize it)
network.port_range.set = 50000-50000
network.port_random.set = no

# Tracker-less torrent and UDP tracker support
# (conservative settings for 'private' trackers, change for 'public')
dht.mode.set = disable
protocol.pex.set = no
trackers.use_udp.set = no

# Peer settings
throttle.min_peers.normal.set = 20
throttle.max_peers.normal.set = 60
throttle.min_peers.seed.set = 30
throttle.max_peers.seed.set = 80

# Limits for file handle resources, this is optimized for
# an `ulimit` of 1024 (a common default). You MUST leave
# a ceiling of handles reserved for rTorrent's internal needs!
network.http.max_open.set = 50
network.max_open_files.set = 600
network.max_open_sockets.set = 300

# Memory resource usage (increase if you have a large number of items loaded,
# and/or the available resources to spend)
pieces.memory.max.set = 1800M
network.xmlrpc.size_limit.set = 2M

# Basic operational settings (no need to change these)
session.path.set = (cat,(cfg.basedir),".session")
directory.default.set = (cat,(cfg.basedir),"download")

# Watch directories (add more as you like, but use unique schedule names)
schedule = watch_start,10,10,((load.start,(cat,(cfg.watch),"start/*.torrent")))
schedule = watch_load,15,10,((load.normal,(cat,(cfg.watch),"load/*.torrent")))

# Logging:
#   Levels = critical error warn notice info debug
#   Groups = connection_* dht_* peer_* rpc_* storage_* thread_* tracker_* torrent_*
print = (cat,"Logging to ",(cfg.logfile))
log.open_file = "log", (cfg.logfile)
log.add_output = "info", "log"
#log.add_output = "tracker_debug", "log"

### END of rtorrent.rc ###
```