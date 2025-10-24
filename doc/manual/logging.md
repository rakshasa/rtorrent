# Logging

## Opening log files

    # log.open_file = "log name", "file path"

    log.open_file = "rtorrent.log", (cat,/tmp/rtorrent.log.,(system.pid))

A newly opened log file is not connected to any logging events.

Some control over formatting will be provided at a later date.

## Appending to log files

    log.append_file = "rtorrent.log", "/tmp/rtorrent.log"

The `log.open_file` clears any existing contents of the file. If you'd
prefer to have a single log file that persists across application
restarts, you can use the `log.append_file` in place of the
`log.open_file` configuration key.

## Adding outputs to events

    # log.add_output = "logging event", "log name"

    log.add_output = "info", "rtorrent.log"

    log.add_output = "dht_all", "tracker.log"
    log.add_output = "tracker_events", "tracker.log"
    log.add_output = "tracker_requests", "tracker.log"

Each log handle can be added to multiple different logging events.

## Logging events

    "critical"
    "error"
    "warn"
    "notice"
    "info"
    "debug"

The above events receive logging events from all the sub-groups
displayed below, and each event also receiving events from the event
above in importance.

Thus some high-volume sub-group events such as “tracker\_debug” are not
part of “debug” and every “warn” event will receive events from “error”,
“critical”.

    "connection_*"
    "dht_*"
    "peer_*"
    "rpc_*"
    "storage_*"
    "thread_*"
    "tracker_*"
    "torrent_*"

All sub-groups have events from “critical” to “debug” defined.
