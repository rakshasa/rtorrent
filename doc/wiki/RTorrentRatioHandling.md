# rTorrent Ratio Handling

The ratio handling has been updated in rtorrent 0.8.4. Page originally from https://web.archive.org/web/20140219224509/http://libtorrent.rakshasa.no/wiki/RTorrentRatioHandling

# Quick setup

Here is a working config that you can use as a starting point.

Use the 'max', 'min' and 'upload' variables as in the old version.

```
# Enable the default ratio group.
ratio.enable=

# Change the limits, the defaults should be sufficient.
ratio.min.set=100
ratio.max.set=300
ratio.upload.set=20M

# Changing the command triggered when the ratio is reached.
system.method.set = group.seeding.ratio.command, d.close=, d.erase=
```

# The basics

```
# Default group for ratio handling.
group.seeding.view
group.seeding.ratio.command
group.seeding.ratio.disable
group.seeding.ratio.enable
group.seeding.ratio.max
group.seeding.ratio.max.set
group.seeding.ratio.min
group.seeding.ratio.min.set
group.seeding.ratio.upload
group.seeding.ratio.upload.set

# The above commands can be called through:
ratio.disable
ratio.enable
ratio.max
ratio.max.set
ratio.min
ratio.min.set
ratio.upload
ratio.upload.set
```

The 'group.seeding.view' variable points to the 'seeding' view, which contains all seeding downloads, and the default 'ratio.command' closes the download. To enabled or disable the ratio handling for the group, call the 'enable' and 'disable' commands which automatically adds it to the scheduler.

# Ratio groups

If you wish to specify different ratio's for different watch directories, do the following:

```
# Add new views. You may find out what downloads they contain through
# 'ui.current_view.set=group_1' command or XMLRPC calls.
view_add = view_group_1

# Make the views persist across sessions.
view.persistent = view_group_1

# Create new groups, 'group.insert = <name>, <view>'.
group.insert = group_1, view_group_1

group.group_1.ratio.enable=
group.group_1.ratio.min.set=100
group.group_1.ratio.max.set=300
group.group_1.ratio.upload.set=20M 

# Optionally you may create a persistent view group directly. Note
# that the view name is the same as the group name.
group.insert_persistent_view = group_2

group.group_2.ratio.enable=
group.group_2.ratio.min.set=300
group.group_2.ratio.max.set=0

# Downloads need to be inserted into the view with the 'view.set_visible'
# command. Note that extra parameters to 'load' are commands called
# with the newly created download as the target.
schedule = watch_directory_1,5,10,"load_start_verbose=foo_1/*.torrent, view.set_visible=view_group_1"
schedule = watch_directory_2,5,10,"load_start_verbose=foo_2/*.torrent, view.set_visible=group_2"
```