# Gnuplot script file for plotting data in file "data.dat"
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set title "Time to sort vs number of threads"
set xlabel "Time to sort (milliseconds)"
set ylabel "Number of Threads"
set style data linespoints
set term png
set output filename
plot "data.dat" t "time to sort"