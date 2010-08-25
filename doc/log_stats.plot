# $system.time_seconds=
#
# $throttle.global_up.rate=
# $throttle.global_up.total=
# $throttle.global_down.rate=
# $throttle.global_down.total=
#
# $throttle.unchoked_uploads=
# $throttle.unchoked_downloads=
# $pieces.memory.current=
#
# schedule = log_stats,5,10,"execute={log_rtorrent.sh,/Users/rakshasa/tmp/rtorrent.log,$system.time_seconds=,$throttle.global_up.rate=,$throttle.global_up.total=,$throttle.global_down.rate=,$throttle.global_down.total=,$throttle.unchoked_uploads=,$throttle.unchoked_downloads=,$pieces.memory.current=}"


set terminal png
set output "output.png"
set xdata time
set timefmt "%s"
set format x "%H:%M"

#set multiplot

plot "./rtorrent.log" using 1:2 smooth bezier with lines title 'Upload rate',\
     "./rtorrent.log" using 1:4 smooth bezier with lines title 'Download rate',\
     "./rtorrent.log" using 1:6 smooth bezier with lines title 'Upload unchoked' axis x1y2,\
     "./rtorrent.log" using 1:7 smooth bezier with lines title 'Download unchoked' axis x1y2

set output "output_2.png"

plot "./rtorrent.log" using 1:8 smooth bezier with lines title 'Memory usage' axis x1y1

set output "output_incore.png"

plot "/tmp/rtorrent.incore.8032" using 1:2 smooth bezier with lines title 'Incore cont.' axis x1y1,\
     "/tmp/rtorrent.incore.8032" using 1:4 smooth bezier with lines title 'Not incore cont.' axis x1y1,\
     "/tmp/rtorrent.incore.8032" using 1:3 smooth bezier with lines title 'Incore new' axis x1y1,\
     "/tmp/rtorrent.incore.8032" using 1:5 smooth bezier with lines title 'Not incore new' axis x1y1
