#/bin/bash
gnuplot << EOF
# 1)  system.time_seconds=
#
# 2)  throttle.global_up.rate
# 3)  throttle.global_up.total
# 4)  throttle.global_down.rate
# 5)  throttle.global_down.total
#
# 6)  pieces.memory.current
# 7)  pieces.memory.sync_queue
# 8)  pieces.memory.block_count
# 9)  pieces.stats.total_size
# 10) pieces.hash.queue_size
#
# file.append = (cat,/foo/bandwidth_stats.,(system.pid)),"timestamp","\"upload rate\"","\"upload total\"","\"download rate\"","\"download total\"","\"memory usage\"","\"sync queue\"","\"block count\"","\"total size\"","\"hash queue size\""
# schedule = log_bandwidth_stats,5,10,((file.append,((cat,/foo/bandwidth_stats.,((system.pid)))),((system.time_seconds)),((throttle.global_up.rate)),((throttle.global_up.total)),((throttle.global_down.rate)),((throttle.global_down.total)),((pieces.memory.current)),((pieces.memory.sync_queue)),((pieces.memory.block_count)),((pieces.stats.total_size)),((pieces.hash.queue_size))))
#
# 1) system.time_seconds
#
# 2) throttle.unchoked_uploads
# 3) throttle.unchoked_downloads
# 4) network.open_sockets
#
# 5) view.size,leeching
# 6) view.size,seeding
# 7) view.size,active
#
# file.append = (cat,/foo/peer_stats.,(system.pid)),"timestamp","\"upload unchoked\"","\"download unchoked\"","\"open sockets\"","leeching","seeding","active"
# schedule = log_peer_stats,5,10,((file.append,((cat,/foo/peer_stats.,((system.pid)))),((system.time_seconds)),((throttle.unchoked_uploads)),((throttle.unchoked_downloads)),((network.open_sockets)),((view.size,leeching)),((view.size,seeding)),((view.size,active))))
#
# Choke groups:
#
# file.append = (cat,/foo/choke_group_stats.,(system.pid)),"timestamp",\
# "\"default up unchoked\"","\"default up queued\"","\"default up rate\"",\
# "\"default down unchoked\"","\"default down queued\"","\"default down rate\"","\"default torrents\"",\
# "\"leech up unchoked\"","\"leech up queued\"","\"leech up rate\"",\
# "\"leech down unchoked\"","\"leech down queued\"","\"leech down rate\"","\"leech torrents\"",\
# "\"seed boost up unchoked\"","\"seed boost up queued\"","\"seed boost up rate\"",\
# "\"seed boost down unchoked\"","\"seed boost down queued\"","\"seed boost down rate\"","\"seed boost torrents\"",\
# "\"seed up unchoked\"","\"seed up queued\"","\"seed up rate\"",\
# "\"seed down unchoked\"","\"seed down queued\"","\"seed down rate\"","\"seed torrents\""
#
# schedule = log_choke_group_stats,5,10,((file.append,((cat,/foo/choke_group_stats.,((system.pid)))),((system.time_seconds)),\
# ((choke_group.up.unchoked,0)),((choke_group.up.queued,0)),((choke_group.up.rate,0)),((choke_group.down.unchoked,0)),((choke_group.down.queued,0)),((choke_group.down.rate,0)),((choke_group.size,0)),\
# ((choke_group.up.unchoked,1)),((choke_group.up.queued,1)),((choke_group.up.rate,1)),((choke_group.down.unchoked,1)),((choke_group.down.queued,1)),((choke_group.down.rate,1)),((choke_group.size,1)),\
# ((choke_group.up.unchoked,2)),((choke_group.up.queued,2)),((choke_group.up.rate,2)),((choke_group.down.unchoked,2)),((choke_group.down.queued,2)),((choke_group.down.rate,2)),((choke_group.size,2)),\
# ((choke_group.up.unchoked,3)),((choke_group.up.queued,3)),((choke_group.up.rate,3)),((choke_group.down.unchoked,3)),((choke_group.down.queued,3)),((choke_group.down.rate,3)),((choke_group.size,3)) ))
#
# Mincore stats:
#
# file.append = (cat,/foo/mincore_stats.,(system.pid)),"timestamp","\"incore cont. /10s\"","\"not incore cont. /10s\"","\"incore new /10s\"","\"not incore new /10s\"","\"incore break /10s\"","\"sync success /10s\"","\"sync failed /10s\"","\"sync not synced /10s\"","\"sync not deallocated /10s\"","\"alloc failed /10s\"","\"allocate velocity /10s\"","\"deallocate velocity /10s\""
# log.libtorrent = mincore_stats,(cat,"/foo/mincore_stats.",(system.pid))


set terminal png size 1024,768 enhanced
#set terminal png size 2*1024,768 enhanced
set xdata time
set timefmt "%s"
set format x "%H:%M"
set format y2 "%.0f"
set y2tics
set autoscale xfix
set key autotitle columnhead

set output "output_$1_sockets.png"
plot "peer_stats.$1" using 1:7 smooth sbezier with lines lw 4,\
     "peer_stats.$1" using 1:5 smooth sbezier with lines lw 4,\
     "peer_stats.$1" using 1:6 smooth sbezier with lines lw 4,\
     "<awk '{x=1000; if (\$4 < 1000) x=\$4; print \$1,x}' peer_stats.$1" using 1:2 smooth sbezier with lines title "open sockets" lw 2 axis x1y2,\
     "peer_stats.$1" using 1:2 smooth sbezier with lines lw 2 axis x1y2,\
     "peer_stats.$1" using 1:3 smooth sbezier with lines lw 2 axis x1y2

set output "output_$1_groups.png"
plot "choke_group_stats.$1" \
        using 1:8     smooth sbezier with lines lw 4,\
     "" using 1:8+7*1 smooth sbezier with lines lw 4,\
     "" using 1:8+7*2 smooth sbezier with lines lw 4,\
     "" using 1:8+7*3 smooth sbezier with lines lw 4\


set format y "%.0s %cb"

set output "output_$1_bandwidth.png"
plot "bandwidth_stats.$1" using 1:2 smooth sbezier with lines lw 4,\
     "bandwidth_stats.$1" using 1:4 smooth sbezier with lines lw 4,\
     "choke_group_stats.$1" using 1:4+7*1 smooth sbezier with lines lw 3,\
     "choke_group_stats.$1" using 1:7+7*1 smooth sbezier with lines lw 3,\
     "choke_group_stats.$1" using 1:4+7*2 smooth sbezier with lines lw 3,\
     "choke_group_stats.$1" using 1:4+7*3 smooth sbezier with lines lw 3,\
     "peer_stats.$1" using 1:2 smooth sbezier with lines lw 2 axis x1y2,\
     "peer_stats.$1" using 1:3 smooth sbezier with lines lw 2 axis x1y2

set output "output_$1_memory.png"
plot "bandwidth_stats.$1" using 1:6 smooth sbezier with lines lw 4 axis x1y1,\
     "peer_stats.$1" using 1:2 smooth sbezier with lines lw 2 axis x1y2,\
     "peer_stats.$1" using 1:3 smooth sbezier with lines lw 2 axis x1y2

#set yrange [0:3900000000.0]
#set y2range [0:2000]

set output "output_$1_pieces.png"
plot "bandwidth_stats.$1" using 1:6   smooth sbezier with lines lw 4 axis x1y1,\
     "bandwidth_stats.$1" using 1:7   smooth sbezier with lines lw 4 axis x1y1,\
     "<awk '{x=\$12*10; print \$1,x}' mincore_stats.$1" using 1:2 smooth sbezier with lines lw 4 title 'alloc /100s' axis x1y1,\
     "<awk '{x=\$13*10; print \$1,x}' mincore_stats.$1" using 1:2 smooth sbezier with lines lw 4 title 'dealloc /100s' axis x1y1,\
     "mincore_stats.$1"   using 1:7   smooth sbezier with lines lw 2 axis x1y2,\
     "mincore_stats.$1"   using 1:8   smooth sbezier with lines lw 2 axis x1y2,\
     "mincore_stats.$1"   using 1:11  smooth sbezier with lines lw 2 axis x1y2,\
     "<awk '{x=\$8/10; print \$1,x}'  bandwidth_stats.$1" using 1:2 smooth sbezier with lines lw 2 title 'block count /10' axis x1y2,\
     "bandwidth_stats.$1" using 1:10  smooth sbezier with lines lw 2 axis x1y2

#set yrange [*:*]
#set y2range [*:*]

set format y2 "%.0s %cb"
set output "output_$1_torrents.png"
plot "bandwidth_stats.$1" using 1:6 smooth sbezier with lines lw 4 axis x1y1,\
     "bandwidth_stats.$1" using 1:9 smooth sbezier with lines lw 2 axis x1y2

set format y "%.0f"
set format y2 "%.0f"

set output "output_$1_incore.png"
#plot "bandwidth_stats.$1" using 1:(0) smooth sbezier with lines,
plot \
     "mincore_stats.$1" using 1:2 smooth sbezier with lines lw 2 axis x1y1,\
     "mincore_stats.$1" using 1:3 smooth sbezier with lines lw 4 axis x1y2,\
     "mincore_stats.$1" using 1:4 smooth sbezier with lines lw 2 axis x1y1,\
     "mincore_stats.$1" using 1:5 smooth sbezier with lines lw 4 axis x1y2

#
# Updated:
#

set output "output_$1_choke.png"
set style histogram columnstacked

plot "choke_group_stats.$1" \
        using 1:2+7*1 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:3+7*1 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:5+7*1 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:6+7*1 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:2+7*2 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:3+7*2 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:2+7*3 smooth sbezier with lines lw 2 axis x1y2,\
     "" using 1:3+7*3 smooth sbezier with lines lw 2 axis x1y2

EOF
