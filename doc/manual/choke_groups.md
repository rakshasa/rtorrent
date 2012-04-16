# Choke Groups

## Settings

    choke_group.insert = "leech_fast"

Create a new group named "leech_fast", accessible by the string or
index / reverse index according to order of insertion. E.g. '-1'
refers to the last inserted choke group, while 0 refers to the first.

    choke_group.tracker.mode.set = -1,"aggressive"

Set the tracker mode for torrents in this group.

    choke_group.up.heuristics.set = -1,"upload_leech_experimental"
    choke_group.down.heuristics.set = -1,"download_leech"

Set the heuristics used when deciding on which peers to choke and
unchoke. Use "strings.choke_heuristics{,.upload,.download}" to get a
list of the available heuristics.

    choke_group.up.max.set = -1,250
    choke_group.down.max.set = -1,500

Set the max total number of unchoked peers for all torrents in this choke group.
