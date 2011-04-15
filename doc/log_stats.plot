#/bin/bash
gnuplot << EOF
# 1)  $system.time_seconds=
#
# 2)  $throttle.global_up.rate=
# 3)  $throttle.global_up.total=
# 4)  $throttle.global_down.rate=
# 5)  $throttle.global_down.total=
#
# 6)  $throttle.unchoked_uploads=
# 7)  $throttle.unchoked_downloads=
# 8)  $view.size=active
# 9)  $pieces.memory.current=
# 10) $pieces.memory.sync_queue=
# 11) $pieces.memory.block_count=
# 12) $pieces.stats.total_size=
# 13) $pieces.hash.queue_size=
#
# log.libtorrent = mincore_stats,(cat,"/foo/mincore_stats.",(system.pid))
# schedule = log_stats,5,10,((execute,log_rtorrent.sh,((cat,/foo/bandwidth_stats.,((system.pid)))),((system.time_seconds)),((throttle.global_up.rate)),((throttle.global_up.total)),((throttle.global_down.rate)),((throttle.global_down.total)),((throttle.unchoked_uploads)),((throttle.unchoked_downloads)),((view.size,active)),((pieces.memory.current)),((pieces.memory.sync_queue)),((pieces.memory.block_count)),((pieces.stats.total_size)),((pieces.hash.queue_size))))


set terminal png size 1024,600
set xdata time
set timefmt "%s"
set format x "%H:%M"
set format y "%.0s %cb"
set format y2 "%.0f"
set y2tics
set autoscale xfix

grab(x)=(x<min)?min=x:(x>max)?max=x:0;
#div100(x)=(x/100)
divrak(v)=(v>0)?-v:0;

set output "output_$1_bandwidth.png"
plot "bandwidth_stats.$1" using 1:2 smooth bezier with lines lw 4 title 'Upload rate',\
     "bandwidth_stats.$1" using 1:4 smooth bezier with lines lw 4 title 'Download rate',\
     "bandwidth_stats.$1" using 1:6 smooth bezier with lines lw 2 title 'Upload unchoked' axis x1y2,\
     "bandwidth_stats.$1" using 1:7 smooth bezier with lines lw 2 title 'Download unchoked' axis x1y2

set output "output_$1_memory.png"
plot "bandwidth_stats.$1" using 1:9 smooth bezier with lines lw 4 title 'Memory usage' axis x1y1,\
     "bandwidth_stats.$1" using 1:6 smooth bezier with lines lw 2 title 'Upload unchoked' axis x1y2,\
     "bandwidth_stats.$1" using 1:7 smooth bezier with lines lw 2 title 'Download unchoked' axis x1y2

#set yrange [0:3900000000.0]
#set y2range [0:2000]

set output "output_$1_pieces.png"
plot "bandwidth_stats.$1" using 1:9  smooth bezier with lines lw 4 title 'Memory usage' axis x1y1,\
     "bandwidth_stats.$1" using 1:10 smooth bezier with lines lw 4 title 'Sync queue Memory' axis x1y1,\
     "<awk '{x=\$12*10; print \$1,x}' mincore_stats.$1" using 1:2 smooth bezier with lines lw 4 title 'Alloc /100s' axis x1y1,\
     "<awk '{x=\$13*10; print \$1,x}' mincore_stats.$1" using 1:2 smooth bezier with lines lw 4 title 'Dealloc /100s' axis x1y1,\
     "mincore_stats.$1"   using 1:7  smooth bezier with lines lw 2 title 'Sync success /10s' axis x1y2,\
     "mincore_stats.$1"   using 1:8  smooth bezier with lines lw 2 title 'Sync failed /10s' axis x1y2,\
     "mincore_stats.$1"   using 1:11  smooth bezier with lines lw 2 title 'Alloc failed /10s' axis x1y2,\
     "<awk '{x=\$11/10; print \$1,x}' bandwidth_stats.$1" using 1:2 smooth bezier with lines lw 2 title 'Block count /10' axis x1y2


#set yrange [*:*]
#set y2range [*:*]

set format y2 "%.0s %cb"
set output "output_$1_torrents.png"
plot "bandwidth_stats.$1" using 1:9 smooth bezier with lines lw 4 title 'Memory usage' axis x1y1,\
     "bandwidth_stats.$1" using 1:12 smooth bezier with lines lw 2 title 'Pieces total size' axis x1y2

set format y "%.0f"

set output "output_$1_incore.png"
plot "bandwidth_stats.$1" using 1:(0) smooth bezier with lines title '',\
     "mincore_stats.$1" using 1:2 smooth bezier with lines lw 2 title 'Incore cont.' axis x1y1,\
     "mincore_stats.$1" using 1:3 smooth bezier with lines lw 4 title 'Incore new' axis x1y2,\
     "mincore_stats.$1" using 1:4 smooth bezier with lines lw 2 title 'Not incore cont.' axis x1y1,\
     "mincore_stats.$1" using 1:5 smooth bezier with lines lw 4 title 'Not incore new' axis x1y2

EOF
