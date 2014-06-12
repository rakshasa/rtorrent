#/bin/bash
gnuplot << EOF

set terminal png size 1024,768 enhanced
set xdata time
set timefmt "%s"
set format x "%H:%M"
set y2tics
set autoscale xfix
set key autotitle columnhead

# set datafile missing '0'

set format y "%.0f"
set format y2 "%.0f"

set yrange [0:]
set y2range [0:]

set output "output_$1_transfer_requests.png"
plot \
     "instrumentation_transfers.log.$1" using 1:2  title 'delegated'       with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:3  title 'downloading'     with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:4  title 'finished'        with lines lw 4 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:5  title 'skipped'         with lines lw 2 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:6  title 'unknown'         with lines lw 2 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:7  title 'unordered'       with lines lw 2 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:19 title 'unaccounted'     with lines lw 2 axis x1y2

set output "output_$1_transfer_queues.png"
plot \
     "instrumentation_transfers.log.$1" using 1:8  title 'queue added'       with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:9  title 'queue moved'       with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:10 title 'queue removed'     with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:11 title 'queue total'       with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:12 title 'unordered added'   with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:13 title 'unordered moved'   with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:14 title 'unordered removed' with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:15 title 'unordered total'   with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:16 title 'stalled added'     with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:17 title 'stalled moved'     with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:18 title 'stalled removed'   with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:19 title 'stalled total'     with lines lw 4 axis x1y2,\
     "instrumentation_transfers.log.$1" using 1:20 title 'choked added'      with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:21 title 'choked moved'      with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:22 title 'choked removed'    with lines lw 2 axis x1y1,\
     "instrumentation_transfers.log.$1" using 1:23 title 'choked total'      with lines lw 4 axis x1y2

EOF
