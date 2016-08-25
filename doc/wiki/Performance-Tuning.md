# rTorrent Performance Tuning
**Contents**

 * [rTorrent related settings](#rtorrent-related-settings)
  * [Sample config entries](#sample-config-entries)
  * [Peers and slots](#peers-and-slots)
    * [Definitions](#definitions)
    * [Assigning the right values](#assigning-the-right-values)
    * [Memory impact](#memory-impact)
  * [Max memory usage setting](#max-memory-usage-setting)
  * [Reduce disk usage](#reduce-disk-usage)
    * [Send and receive buffer size](#send-and-receive-buffer-size)
    * [Preloading pieces](#preloading-pieces)
    * [Max open files](#max-open-files)
    * [Disk allocation](#disk-allocation)
    * [Session save](#session-save)
  * [DNS timeout](#dns-timeout)
 * [System wide settings](#system-wide-settings)
  * [Max open files](#max-open-files)
  * [Networking tweaks](#networking-tweaks)
 * [Name resolving enhancements](#name-resolving-enhancements)
  * [rTrorrent with c-ares](#rtrorrent-with-c-ares)
  * [Local DNS cache](#local-dns-cache)
 * [References](#references)


## rTorrent related settings

Assuming everything is going well we can focus on performance tuning.

### Sample config entries

Let's see the related possible settings at once first. These settings are used with 74/20 Mbps connection, 4 GB RAM and 1 local disk device and with `rTorrent v0.9.6`.

```ini
# Global upload and download rate in KiB, `0` for unlimited (`download_rate`, `upload_rate`)
throttle.global_down.max_rate.set_kb = 8700
throttle.global_up.max_rate.set_kb   = 2200

# Maximum number of simultaneous downloads and uploads slots (global slots!) (`max_downloads_global`, `max_uploads_global`)
throttle.max_downloads.global.set = 300
throttle.max_uploads.global.set   = 300

# Maximum and minimum number of peers to connect to per torrent while downloading (`min_peers`, `max_peers`) Default: `100` and `200` respectively
throttle.min_peers.normal.set = 99
throttle.max_peers.normal.set = 100

# Same as above but for seeding completed torrents (seeds per torrent), `-1` for same as downloading (`min_peers_seed`, `max_peers_seed`) Default: `-1` for both
throttle.min_peers.seed.set = -1
throttle.max_peers.seed.set = -1

# Maximum number of simultaneous downloads and uploads slots per torrent (`max_uploads`) Default: `50` for both
throttle.max_downloads.set = 50
throttle.max_uploads.set = 50

# Set the numwant field sent to the tracker, which indicates how many peers we want. 
#  A negative value disables this feature. Default: `-1` (`tracker_numwant`)
trackers.numwant.set = 100

# Set the max amount of memory address space used to mapping file chunks. This refers to memory mapping, not
#  physical memory allocation. Default: `1GB` (`max_memory_usage`) 
# This may also be set using ulimit -m where 3/4 will be allocated to file chunks.
pieces.memory.max.set = 2048M

# Maximum number of connections rtorrent can accept/make (`sockets`)
network.max_open_sockets.set = 999

# Maximum number of open files rtorrent can keep open (you have to modify the system wide settings with ulimit!) (`set_max_open_files`)
network.max_open_files.set = 600

# Maximum number of simultaneous HTTP request (used by announce or scrape requests) Default: `32` (`set_max_open_http`)
network.http.max_open.set = 99

# Send and receive buffer size for socket. Disabled by default (`0`), this means the default is used by OS 
#  (you have to modify the system wide settings!) (`send_buffer_size`, `receive_buffer_size`)
# Increasing buffer sizes may help reduce disk seeking, connection polling as more data is buffered each time
#  the socket is written to. It will result higher memory usage (not visible in rtorrent process!).
network.receive_buffer.size.set =  4M
network.send_buffer.size.set    = 12M

# Preloading a piece of a file. Default: `0` Possible values: `0` (Off) , `1` (Madvise) , `2` (Direct paging).
pieces.preload.type.set = 2
#pieces.preload.min_size.set = 262144
#pieces.preload.min_rate.set = 5120

# TOS of peer connections. Default: `throughput`. If the option is set to `default` then the system default TOS
#  is used. A hex value may be used for non-standard settings.  (`tos`)
# Possible values: `[default|lowdelay|throughput|reliability|mincost]` or a hex value.
#network.tos.set = throughput

# CURL options to add support for nonofficial SSL trackers and peers
network.http.ssl_verify_host.set = 0
network.http.ssl_verify_peer.set = 0

# CURL option to lower DNS timeout. Default: `60`.
network.http.dns_cache_timeout.set = 25

# Max packet size using xmlrpc. Default: `524288` (xmlrpc_size_limit)
network.xmlrpc.size_limit.set = 2M

# Save all the sessions in every 12 hours instead of the default 20 minutes.
schedule2 = session_save, 1200, 43200, ((session.save))

# Prune file status in every 24 hours, this is the default setting.
#schedule2 = prune_file_status, 3600, 86400, ((system.file_status_cache.prune))

# Whether to allocate disk space for a new torrent. Default: `0`
#system.file.allocate.set = 0
```

### Peers and slots

`rTorrent` uses a different philosophy than most other torrent client. 

#### Definitions

`slots` - can be upload or download `slots` - determine how many `peers` can actually transfer data at the same time, while `rTorrent` can be connected with way more `peers`. The allowed numbers of `connected peers` should be 2 or 3 times higher than the allowed number of `slots`.

- `throttle.max_downloads.global`, `throttle.max_uploads.global`: maximum number of global simultaneous downloads and uploads slots. These values limit the global slots, can be seen at the right part of status bar: `[U 45/300] [D 179/300]`
- `throttle.max_uploads`, `throttle.max_downloads`: maximum number of simultaneous downloads and uploads slots per torrent. It can be seen at the bottom left a torrent details page (using right arrow): `Slots: U:0/50 D:0/50`
- `throttle.max_peers.normal`, `throttle.max_peers.seed`: maximum number of peers to connect to per torrent while downloading or seeding. It can be seen (along with the connected peers) at the bottom left a torrent details page (using right arrow): `Peers: 43(0) Min/Max: 99/100`
- `throttle.min_peers.normal`, `throttle.min_peers.seed`: minimum number of peers to connect to per torrent while downloading or seeding. It can be seen (along with the connected peers) at the bottom left a torrent details page (using right arrow): `Peers: 43(0) Min/Max: 99/100`

The `min_peers` values are responsible for asking more peers during an announce request. When the client has less than `min_peers` connections for a download, it will attempt to request more from available trackers. 30 seconds after a request the client will attempt another if more than 10 new peer connections were gained or less than 3 requests have been performed. Else it will try the next tracker group in the list, but not other trackers in the same group. This behavior should give enough peers while minimizing the number of tracker requests, although it will use somewhat longer time than other more aggressive clients. In theory, if these values are `0` then rTorrent won't ask for new peers from the given tracker, peers can still connect though.

#### Assigning the right values

It all depends on the global connection speeds (`throttle.global_down.max_rate`, `throttle.global_up.max_rate`) and available RAM.

1. every upload slot should have got at least `5 KiB/s` speed left (it's not really problem anymore with nowdays fast connection). Taking the above example, in the worst case download slots have `29 KiB/s` (`8700/300`) and upload slots have `7.3 KiB/s` (`2200/300`). That means the number of the global download slot can be increased if we notice that we run out of it.

2. `throttle.max_downloads` and `throttle.max_uploads` slots are `50` now. In the worst case this allows downloading and uploading `6` (`300/50`) torrents at the same time, but this usually not a problem for seeding.

3. `max_peers` settings for downloading and seeding should be at least 2 times higher than the number of slots per torrent, hence the value of `100` for them.

4. `min_peers` settings are `99` for both uploading and downloading, meaning we always want to ask the tracker for new peers.

#### Memory impact

For every download or upload slot you allow you need a chunk's worth of RAM in the system to cache the chunks. E.g. if the current torrent your are uploading has a chunk size of `4 MiB` then you would need `200 MiB` (`4 MiB * 50 slots`) of RAM for `rTorrent` to use. Chunk sizes per torrent range from `256 KiB` to around `8 MiB`.

That means, we need `2.4 GiB` of RAM (`4 MiB * 600 slots`) if the average chunk size is `4 MiB` in the worst case.

### Max memory usage setting

`pieces.memory.max` is commonly misunderstood setting: it doesn't limit or set the amount of RAM `rTorrent` can use (see [Reduce disk usage](#reduce-disk-usage) section below) but limits the amount of memory address space used to mapping file chunks. This refers to memory mapping, not physical memory allocation. Default value is `1 GiB`

That's how can happen that you never see e.g. greater value than `800 MiB` for `RES` in `htop` beside of the `rtorrent` process.

For fast downloads and/or large number of peers this may quickly be exhausted causing the client to hang while it syncs to disk. You may increase this limit.

### Reduce disk usage

In theory, we can reduce disk i/o :) See this issue why: [#443](https://github.com/rakshasa/rtorrent/issues/443)

#### Send and receive buffer size

The `network.send_buffer.size` and `network.receive_buffer.size` options can be used to adjust the socket send and receive buffer sizes. If you set these to a low number, you may see reduced throughput, especially for high latency connections. Increasing buffer sizes may help reduce disk seeking, connection polling as more data is buffered each time the socket is written to. See [Networking tweaks](#networking-tweaks) section how to adjust it system wide.

It affects memory usage: this memory will not be visible in `rtorrent` process, this sets the amount of kernel memory is used for your sockets. In low-memory, high-bandwidth case you still want decent buffer sizes. It is however the number of upload/downloading peers you want to reduce. (Reference: [#435](https://github.com/rakshasa/rtorrent/issues/435#issuecomment-226979078))

#### Preloading pieces

When a piece is to be uploaded to a peer it can preload the piece of the file before it does the non-blocking write to the network. This will not complete the whole piece if parts of the piece is not already in memory, having instead to try again later. (Reference: [#418](https://github.com/rakshasa/rtorrent/issues/418))

`pieces.preload.type` Default: `0`. Possible values:
- `0` = off: it doesn't do any.
- `1` = Madvise: it calls 'madvise' on the file for the specific mmap'ed memory range, which tells the kernel to load it in memory when it gets around to it. Which is hopefully before we write to the network socket.
- `2` = Direct paging: we 'touch' each file page in order to force the kernel to load it into memory. This can help if you're dealing with very large number of peers and large/many files, especially in a low-memory setting, as you can avoid thrashing the disk where loaded file pages get thrown out before they manage to get sent.

Related settings:
```ini
#pieces.preload.min_size.set = 262144
#pieces.preload.min_rate.set = 5120
```

#### Max open files

`network.max_open_files` limits the maximum number of open files `rTorrent` can keep open. By default rTorrent uses variable sized `fd_set`'s depending on the process `sysconf(_SC_OPEN_MAX)` limit. See [Networking tweaks](#networking-tweaks) section how to adjust it system wide.

Large `fd_set`'s cause a performance penalty as they must be cleared each time the client polls the sockets. When using `select` or `epoll` (until `libcurl` is fixed) based polling use an open files limit that is reasonably low (_is it still the case???_). The widely used default of `1024` is enough for most users and `64` is minimum. Those with embeded devices or older platforms might need to set the limit much lower than the default.

(Due to `libcurl`'s use of `fd_set` for polling, `rTorrent` cannot at the moment move to a pure `epoll` implementation. Currently the `epoll` code uses `select` based polling if, and only if, `libcurl` is active. All `non-libcurl` sockets are still in `epoll`, but `select` is used on the `libcurl` and the `epoll`-socket. (_is it still the case???_))

#### Disk allocation

`system.file.allocate` specifies whether to allocate disk space for a new torrent. Default is `0`.

Why would you care about this setting? Because "it’s beneficial in the as-less-fragmentation-as-possible sense".

In short: if you use btrfs, ext4, ocfs2, xfs file system you can enable this without having any performance impact.

When it's enabled (set to `1`):
- on a non-blocking file system that supports `fallocate` (like btrfs, ext4, ocfs2, xfs) that is always used to resize files, hence there is no performance issue (takes only a few microseconds to allocate space for huge files)
- if `fallocate` isn't supported by a blocking file system and `--with-posix-fallocate` was used during compilation of `libtorrent` then that is used to resize files, it can be [significantly slower](https://log.amitshah.net/2009/03/comparison-of-file-systems-and-speeding-up-applications/) then the above one
- on Mac OS X a different method is used
- if none of the above method is available then rtorrent falls back to disabled state, even when it's set to `1`
- if priority of certain files of a download are set to `off` before starting it then file allocation isn't triggered for them

When it's disabled (set to `0`):
- Opening a torrent causes files to be created and resized with `ftruncate` (`ftruncate` has problem on vfat filesystem, though, so another method is used in this case). This does not actually use disk space until data is written, despite what the file sizes are reported as. Use `du` without the flag `--apparent-size` to see the real disk usage.

#### Session save

`rTorrent` saves all the sessions in every 20 minutes by default. With large amount of torrents this can be a disk performance hog (see [#180](https://github.com/rakshasa/rtorrent/issues/180#issuecomment-55140832)).

Increase this interval, e.g. to 12 hours instead, if you think you're system is stable and/or you don't care about the possible loss of resume and local stat in the meantime, with:
```ini
schedule2 = session_save, 1200, 43200, ((session.save))
```

(_What is this for??? `schedule2 = prune_file_status, 3600, 86400, ((system.file_status_cache.prune))`_)


### DNS timeout

Along with [Name resolving enhancements](#name-resolving-enhancements) we can reduce http DNS cache timeout with `network.http.dns_cache_timeout`. Since we set it in [Networking tweaks](#networking-tweaks) to `30`, let's lower it to `25`. Default is `60` sec.


## System wide settings

To use higher settings for couple of the above settings the system wide limit should be raised for them.

### Max open files

`network.max_open_files` is limited to `ulimit -n`. If you want to increase it you can use `ulimit -n new_value` or apply it permanently via `/etc/security/limits.conf` on Ubuntu (default: `1024`):
```ini
#<domain>      <type>  <item>         <value>
username       soft    nofile         10240
username       hard    nofile         10240
```

### Networking tweaks

`network.receive_buffer.size`, `network.send_buffer.size` are limited to `sysctl -a | grep -i rmem` and `sysctl -a | grep -i wmem`. You have to change `net.core.rmem_max`, `net.ipv4.tcp_rmem`, `net.core.wmem_max`, `net.ipv4.tcp_wmem` in `/etc/sysctl.conf` to the desired values along with some other tweaks:
```ini
# Maximum Socket Receive Buffer. 16MB per socket - which sounds like a lot, but will virtually never consume that much. Default: 212992
net.core.rmem_max = 16777216
# Maximum Socket Send Buffer. 16MB per socket - which sounds like a lot, but will virtually never consume that much. Default: 212992
net.core.wmem_max = 16777216
# Increase the write-buffer-space allocatable: min 4KB, def 12MB, max 16MB. Default: 4096 16384 4194304
net.ipv4.tcp_wmem = 4096 12582912 16777216
# Increase the read-buffer-space allocatable: min 4KB, def 12MB, max 16MB. Default: 4096 16384 4194304
net.ipv4.tcp_rmem = 4096 12582912 16777216

# Tells the system whether it should start at the default window size only for new TCP connections or also for existing TCP connections that have been idle for too long. Default: 1
net.ipv4.tcp_slow_start_after_idle = 0
# Allow reuse of sockets in TIME_WAIT state for new connections only when it is safe from the network stack’s perspective. Default: 0
net.ipv4.tcp_tw_reuse = 1
# Do not last the complete time_wait cycle. Default: 0
net.ipv4.tcp_tw_recycle = 1
# Minimum time a socket will stay in TIME_WAIT state (unusable after being used once). Default: 60
net.ipv4.tcp_fin_timeout = 30
```


## Name resolving enhancements

`rTorrent` sometimes can hang on hostname lookups, even with normal http/https requests. Here it is what we can do about it.

### rTrorrent with c-ares

`c-ares` is a C library for asynchronous DNS requests (including name resolves). [Here](http://web.archive.org/web/20140727040445/http://filesharefreak.com/tutorials/rtorrent-libtorrent-installing-on-linux) you can find instructions how to compile it or just simply use [rtorrent-ps](https://github.com/pyroscope/rtorrent-ps) by @pyroscope, its build script will do everything you need.

### Local DNS cache

In addition to the above, it's strongly advised to use DNS caching either locally or for your whole network (e.g. on a OpenWRT powered router). [Here](https://help.ubuntu.com/community/Dnsmasq#Local_DNS_Cache) you can find instructions how to set it up on Ubuntu.

## References

- [libtorrent manual](http://www.libtorrent.org/tuning.html)
- [lost rTorrent docs](http://web.archive.org/web/20131104130853/http://libtorrent.rakshasa.no/wiki/RTorrentPerformanceTuning)
- [rTorrent research](https://calomel.org/rtorrent_mods.html)
- [c-ares](http://c-ares.haxx.se)
- [Local DNS Cache](https://help.ubuntu.com/community/Dnsmasq#Local_DNS_Cache)
- [Networking tweaks](http://wiki.mikejung.biz/Sysctl_tweaks)