# Produces four PNG graphs of latency results from the "bench" example program.
#
# Output filenames:
#
#     latency-small.png : Graph of small message payloads
#     latency-medium.png : Graph of medium message payloads
#     latency-large.png : Graph of large message payloads
#     latency-all.png : Graph of all the data
#
# Usage:
#   gnuplot -e "filename='MY_FILENAME'; prefix='MY_PREFIX'" latency.gp
#
#   where prefix is a string that will be prepended to the title. This can
#   be used to describe the test scenario.


set terminal png size 1920,1080 enhanced font 'Verdana, 20'
set ylabel 'microseconds'
set xlabel 'Message size (bytes)'
set grid
set xtics (0, '256' 1, '512' 2, "1K" 3, "2K" 4, "4K" 5, "8K" 6, "16K" 7,\
           "32K" 8, "64K" 9, "128K" 10, "256K" 11, "512K" 12, "1MB" 13,\
           "2MB" 14, "4MB" 15, "8MB" 16)

set output "latency-all.png"
set title sprintf("%s Latency", prefix)
plot filename using 1:3 with linespoints title 'Avg' lw 2

set output "latency-small.png"
set title sprintf("%s Latency with Small Messages", prefix)
set xrange [0:8]
plot filename using 1:3 with linespoints title 'Avg' lw 2

set output "latency-medium.png"
set title sprintf("%s Latency with Medium Messages", prefix)
set xrange [7:13]
plot filename using 1:3 with linespoints title 'Avg' lw 2

set output "latency-large.png"
set title sprintf("%s Latency with Large Messages", prefix)
set xrange [12:16]
plot filename using 1:3 with linespoints title 'Avg' lw 2
