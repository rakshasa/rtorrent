Scheduling
==========

Adding tasks
------------

```
schedule2 = <schedule_id>, start, interval, <commands>
```

Call command every interval seconds, starting from start. An interval of zero calls the task once, while a start of zero calls it immediately. Currently command is forwarded to the option handler. start and interval may optionally use a time format, dd:hh:mm:ss. F.ex to start a task every day at 18:00, use 18:00:00,24:00:00.

<schedule_id> can be whatever name you want to give the scheduled event.

```
schedule2 = <schedule_id>, start, interval, "<commands>"
```

Removing tasks
--------------

```
schedule_remove2 = <schedule_id>
```

Delete id from the scheduler.

Examples
--------

```
schedule2 = foo, 10, 60, load.start=~/Download/watch_old/*.torrent
schedule2 = bar, 10, 60, ((print,"foo ",((system.time_seconds))))
schedule2 = baz, 18:00:00, 24:00:00, ((print,"it's 6pm ",((system.time_seconds))))
```

Setting custom variables on torrents
--------
You can set custom variables to allow for various events to take place automatically as well.

```
schedule2 = foo, 0, 10, "load.start=~/Download/watch_old/*.torrent,\"d.custom.set=incomplete,1\""
method.set_key=event.download.finished, cldvar,"branch=d.custom=incomplete,\"d.custom.set=incomplete,0\""
method.set_key=event.download.erased, rm_files,"branch=d.custom=incomplete,\"execute2={rm,-rf,--,$d.base_path=}\""
method.set_key=event.download.erased, rm_torrent_files,"branch=d.custom=incomplete,d.delete_tied="
```

The above will auto-start any torrent downloaded to the watch directory and set the custom variable "incomplete" to 1 for that download.

When the download finishes, the custom variable "incomplete" will be set to 0.

If the torrent is erased from rTorrent before being finished, then both the torrent file (d.delete_tied=) and the downloaded data ($d.base_path=) will be erased as well. This is useful so that you can more easily manage your files, however please take care to move your torrent data elsewhere if you want to keep it, before removing it from the client, just in case.

Also note in the above that we have wrapped the command in quotes. This is to tell rTorrent that this is a list of commands that need to be run. Additionally, we have escaped quotes \" around the d.custom.set commands. That is because this is also a command itself, and since there are commas inside THIS command, we need to identify that this is a single command within another command.