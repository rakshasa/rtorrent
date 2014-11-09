#/bin/bash
gnuplot << EOF

set terminal png size 1024 * 1, 768 enhanced
set xdata time
set timefmt "%s"
set format x "%H:%M"
set y2tics
set autoscale xfix
set key autotitle columnhead

set format y "%.0f"
set format y2 "%.0f"

set output "output_$1_incore_sync.png"
plot \
     "instrumentation_mincore.log.$1" using 1:6  title 'success'         with lines lw 4 axis x1y1,\
     "instrumentation_mincore.log.$1" using 1:7  title 'failed'          with lines lw 4 axis x1y1,\
     "instrumentation_mincore.log.$1" using 1:8  title 'not synced'      with lines lw 4 axis x1y2,\
     "instrumentation_mincore.log.$1" using 1:9  title 'not deallocated' with lines lw 4 axis x1y2

set format y "%.0g"

set output "output_$1_incore_alloc.png"
plot \
     "instrumentation_mincore.log.$1" using 1:12 title 'allocations'     with lines lw 2 axis x1y1,\
     "instrumentation_mincore.log.$1" using 1:13 title 'deallocations'   with lines lw 4 axis x1y1,\
     "instrumentation_mincore.log.$1" using 1:10 title 'alloc failed'    with lines lw 2 axis x1y2

EOF
