# Choke Groups

## Settings

    (choke_group.insert,"leech_fast")

Create a new group named "leech_fast", accessible by the string or
index / reverse index according to order of insertion. E.g. `-1`
refers to the last inserted choke group, while `0` refers to the first.

`(choke_group.list)` : list of group names

`(choke_group.size)` : number of groups

`(choke_group.index_of,"group_name")` : index in this group, throws if the group name was not found

`(choke_group.general.size,<cg_index>)` : number of torrents in this group

All commands that applies to a group requires the first argument to be
the index, reverse index or the group name.

    (choke_group.tracker.mode,<cg_index>) -> "tracker_mode"
    (choke_group.tracker.mode.set,<cg_index>,"tracker_mode")

Set the tracker mode for torrents in this group. Decide on how aggressive a tracker should be, see
`strings.tracker_mode` for list of available options.

    (choke_group.up.heuristics.set,-1,"upload_leech_experimental")
    (choke_group.down.heuristics.set,-1,"download_leech")

Set the heuristics used when deciding on which peers to choke and
unchoke. Use `strings.choke_heuristics{,_upload,_download}` to get a
list of the available heuristics.

    (choke_group.up.rate,<cg_index>) -> <bytes/second>
    (choke_group.down.rate,<cg_index>) -> <bytes/second>

Upload / download rate for the aggregate of all torrents in this
particular group.

    (choke_group.up.max.set,-1,250)
    (choke_group.down.max.set,-1,500)

Set the max total number of unchoked peers for all torrents in this choke group.

    (choke_group.up.max,<cg_index>) -> <max_upload_slots>
    (choke_group.up.max.unlimited,<cg_index>) -> <max_upload_slots>
    (choke_group.up.max.set,<cg_index>, <max_upload_slots>)
    (choke_group.down.max,<cg_index>) -> <max_download_slots>
    (choke_group.down.max.unlimited,<cg_index>) -> <max_download_slots>
    (choke_group.down.max.set,<cg_index>, <max_download_slots)

Number of unchoked upload / download peers regulated on a group basis.

`(choke_group.up.total,<cg_index>)` : number of queued and unchoked interested peers

`(choke_group.up.queued,<cg_index>)` : number of queued interested peers

`(choke_group.up.unchoked,<cg_index>)` : number of unchoked uploads

`(choke_group.down.total,<cg_index>)` : number of queued and unchoked interested peers

`(choke_group.down.queued,<cg_index>)` : number of queued interested peers

`(choke_group.down.unchoked,<cg_index>)` : number of unchoked uploads


## Downloads settings

`(d.group)` : index of the choke group

`(d.group.name)` : name  of the choke group

`(d.group.set,<cg_index>)` : assign a download to a choke group
