#/bin/bash
gnuplot << EOF

set terminal png size 1024,768 enhanced
set xdata time
set timefmt "%s"
set format x "%H:%M"
set y2tics
set autoscale xfix
set key autotitle columnhead

set datafile missing '0'

set format y "%.0f"
set format y2 "%.0f"

set y2range [0:]

set output "output_$1_transfer_requests.png"
plot \
     "instrumentation_transfers.log.$1" using 1:2  title 'delegated'       smooth sbezier with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:3  title 'downloading'     smooth sbezier with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:4  title 'finished'        smooth sbezier with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:5  title 'skipped'         smooth sbezier with lines lw 2 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:6  title 'unknown'         smooth sbezier with lines lw 2 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:19 title 'unaccounted'     smooth sbezier with lines lw 2 axis x1y2

set output "output_$1_transfer_queues.png"
plot \
     "instrumentation_transfers.log.$1" using 1:7  title 'queue added'       smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:8  title 'queue moved'       smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:9  title 'queue removed'     smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:10 title 'queue total'       smooth sbezier with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:11 title 'unordered added'   smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:12 title 'unordered moved'   smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:13 title 'unordered removed' smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:14 title 'unordered total'   smooth sbezier with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:15 title 'stalled added'     smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:16 title 'stalled moved'     smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:17 title 'stalled removed'   smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:18 title 'stalled total'     smooth sbezier with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:19 title 'choked added'      smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:20 title 'choked moved'      smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:21 title 'choked removed'    smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:22 title 'choked total'      smooth sbezier with lines lw 4 axis x1y2

EOF
