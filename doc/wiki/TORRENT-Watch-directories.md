Watch Directories
=================

Introduction
------------

Using various ways rTorrent can start, stop and delete downloads as torrent files are added or removed from a watch directory.

Scheduled Task
--------------

```
schedule = watch_directory_foo, 10, 10, "load.start=~/watch_foo/*.torrent"
schedule = watch_directory_bar, 10, 10, "load.normal=~/watch_bar/*.torrent"
schedule = watch_directory_baz, 10, 10, "load.normal=~/watch_baz/*.torrent,d.directory.set=~/baz/"
```

The simplest and most portable way to start downloads is through scheduling a task for regular execution.

Tied Files
----------

```
schedule = tied_directory, 10, 10, start_tied=
schedule = untied_directory, 10, 10, stop_untied=
schedule = untied_directory, 10, 10, close_untied=
schedule = untied_directory, 10, 10, remove_untied=
```

When a download is created the torrent file's original path is associated with the download. When the commands '*_untied' are called the respective actions (stop, close, remove) are called for downloads if their original torrent file is not found.

The reverse happens if 'start_tied' is called.

Inotify
-------

On Linux and using v0.9.7+, you may use inotify to watch a directory, and the provided command is called with the the full path of new files as the first argument. Use `method.insert` to define more complex multi-command inotify handlers, or when you want to pass additional parameters to a handler command.

```ini
directory.watch.added = "~/Download/watch/", load.start
```

