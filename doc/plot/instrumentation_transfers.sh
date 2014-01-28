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

set output "output_$1_transfer_requests.png"
plot \
     "instrumentation_transfers.log.$1" using 1:2  title 'downloading'     smooth sbezier with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:3  title 'finished'        smooth sbezier with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:4  title 'skipped'         smooth sbezier with lines lw 2 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:5  title 'unknown'         smooth sbezier with lines lw 2 axis x1y2

set output "output_$1_transfer_queues.png"
plot \
     "instrumentation_transfers.log.$1" using 1:6  title 'queue added'      smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:7  title 'queue removed'    smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:8  title 'queue total'      smooth sbezier with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:9  title 'canceled added'   smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:10 title 'canceled removed' smooth sbezier with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:11 title 'canceled total'   smooth sbezier with lines lw 4 axis x1y2

EOF
