reset
n = 17
max = 25
min = 8
width = (max-min)/n
set xrange [min:max]
set yrange [0:]
set offset graph 0.05,0.05,0.05,0.0
set xtics min,(max-min)/5,max
set boxwidth width*0.9
set style fill solid 0.5
set tics out nomirror
set xlabel "Avg"
set ylabel "Relative Frequency"
s = 5.93446
u = 14.0042
gauss(x) = (1/(sqrt(2*s*pi)))*exp((-(x-u)**2)/(2*s))
plot "data_g.dat" w boxes, gauss(x)
