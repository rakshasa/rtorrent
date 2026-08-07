# Resolving paths

Commands that return a path have `.realpath` variants that resolve it to a
canonical one, with symlinks followed and any `.` or `..` components removed.

The intent is to make paths safer to hand to an external script.
A script called through `execute` receives whatever path rtorrent gives it, and
many scripts do no sanity checking of their own, so resolving the path before it
leaves rtorrent removes a class of surprises: a download directory that is a
symlink into somewhere unexpected, or a torrent whose name walks upwards out of
the directory it is supposed to live in.

    # Instead of this
    execute = ~/bin/on-finished, (d.base_path)

    # Pass the resolved path
    execute = ~/bin/on-finished, (d.base_path.realpath.or_throw)

## Available variants

Each command below comes in an `.or_empty` and an `.or_throw` form.

| Command | Resolves |
| --- | --- |
| `d.base_path.realpath.*` | `d.base_path` |
| `d.directory.realpath.*` | `d.directory` |
| `d.tied_to_file.realpath.*` | `d.tied_to_file` |
| `d.loaded_file.realpath.*` | `d.loaded_file` |
| `f.frozen_path.realpath.*` | `f.frozen_path` |
| `session.path.realpath.*` | `session.path` |
| `directory.default.realpath.*` | `directory.default` |

A leading `~` is expanded first, exactly as it is for `execute`, so
`~/downloads` resolves the same way it would on the command line.

## Paths that do not exist

Resolving requires the path to name an existing file or directory, which is
often not the case for a download whose data has not been written yet.
The two forms differ only in what they do about it.

`.or_empty` returns an empty string:

    print = (d.base_path.realpath.or_empty)    # ""

This keeps `d.multicall` usable over a view that mixes started and unstarted
downloads, since a single unresolvable path does not abort the whole call.
A script receiving one of these paths should still check that it is not empty
before acting on it.

`.or_throw` raises an error instead:

    print = (d.base_path.realpath.or_throw)    # Could not resolve path: '...'

Prefer this one wherever an unresolvable path means the command should not run
at all, such as a single `execute` on `event.download.finished`.

Note that a download only has a file list once it has been opened, so
`d.base_path` and `f.frozen_path` are empty at `event.download.inserted` time
and their `.realpath` variants resolve nothing.
