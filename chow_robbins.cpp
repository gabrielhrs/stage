#include <iostream>
using std::cout; using std::endl;
#include <string>
#include <cmath>
#include <fstream>
#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/students_t.hpp>
#include "utility.hpp"

using namespace std;
#define BUFSIZE 1024
#define BATCHSIZE 8

void plot(int n_int, int n_min, int n_max, double u, double s) {

	ofstream f_plot;
	f_plot.open("plot");

	f_plot << "reset" << endl;

	f_plot << "n = " << n_int << endl; // #number of intervals
	f_plot << "max = " << n_max << endl; //. #max value
	f_plot << "min = " << n_min << endl; // #min value
	f_plot << "width = (max-min)/n" << endl; // #interval width
	//#function used to map a value to the intervals
	//f_plot << "hist(x,width)=width*floor(x/width)+width/2.0" << endl;
	f_plot << "set xrange [min:max]" << endl;
	f_plot << "set yrange [0:]" << endl;
	//#to put an empty boundary around the
	//#data inside an autoscaled graph.
	f_plot << "set offset graph 0.05,0.05,0.05,0.0" << endl;
	f_plot << "set xtics min,(max-min)/5,max" << endl;
	f_plot << "set boxwidth width*0.9" << endl;
	f_plot << "set style fill solid 0.5" << endl; // #fillstyle 
	f_plot << "set tics out nomirror" << endl;
	f_plot << "set xlabel " << "\"Simulation Time\"" << endl;
	f_plot << "set ylabel " << "\"Relative Frequency\"" << endl;
	//#gauss
	f_plot << "s = " << s << endl;
	f_plot << "u = " << u << endl;
	f_plot << "gauss(x) = (1/(sqrt(2*s*pi)))*exp((-(x-u)**2)/(2*s))" << endl;
	//#count and plot
	f_plot << "plot \"data_g.dat\" w boxes, gauss(x)" << endl;
	//f_plot << "plot" << "\"data_g.dat\"" << " u (hist($1,width)):(1.0) smooth freq w boxes lc rgb" << "\"blue\"" << ", gauss(x)" << endl;
	f_plot.close();
}

int main(int argc, char *argv[]) {

	std::cout << "-- Running Chow and Robbins -- " << endl;

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */

	pid_t pid[BATCHSIZE]; /* array with the pid of each process */
	
	int fd[BATCHSIZE][2]; /* array with a file descriptor for each process */

	int child = 0; /* boolean to identify a child */
	int child_idx = -1; /* child's index */

	int g_index = 0; /* number of groups */
	int g_counter; /* a counter to keep track of the number of processes added to a group */

	std::cout << "PLATFORM FILE: " << std::string(platform_file) << endl;
	std::cout << "DEPLOYMENT FILE: " << std::string(deployment_file)  << endl;
	std::cout << "MODEL: " << std::string(model) << endl;

	int k = 0; /* keeps track of the number of dispatched processes */

	/* Chows and Robbins */

	double conf_level = atof(argv[4]);
	double d = atof(argv[5]);

	double error = 1 - conf_level;
	double a = boost::math::erf_inv(1.0 - error/2.0);

	using namespace boost::accumulators;

	accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > acc;

	double d_min = std::numeric_limits<double>::infinity();

	std::multiset<double> classes;

	while(!(d_min <= pow(d,2))) {	
		if(!child) {
			for(g_counter = 0; (g_counter < BATCHSIZE) && !(d_min <= pow(d,2)); g_counter++) { // generates a group of children processes 	
				pipe(fd[k%BATCHSIZE]);
				pid[k%BATCHSIZE] = ::fork();
				if(pid[k%BATCHSIZE] == 0) {
					child_idx = k%BATCHSIZE;
					child = 1;
					break; // child process gets out of the for loop 
				}
				k++;			
			}
		}
		if(child) { // child process calls the code to be simulated and dies 
			close(fd[child_idx][0]);
			std::string const command = "./" + std::string(model) + " " + std::string(platform_file)  
											 + " "  + std::string(deployment_file) 
											 + " " + boost::lexical_cast<string>(fd[child_idx][1])
											 + " " + boost::lexical_cast<string>(k);
			system(command.c_str());
			close(fd[child_idx][1]);
			break; 
		}
		if(!child) { // treats the data of each group 
			wait(NULL);
			int i;
			accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median> > acc_b;

			for (i = g_index * BATCHSIZE; i < k; i++){
	 			char buffer[BUFSIZE] = "";
				waitpid(pid[i%BATCHSIZE], NULL, 0);
				close(fd[i%BATCHSIZE][1]);
				read(fd[i%BATCHSIZE][0], &buffer, sizeof(buffer));
				double v = atof(buffer);
				acc(v);
				classes.insert(floor(v));
				acc_b(v);
				close(fd[i%BATCHSIZE][0]);	
			}

		d_min = ((variance(acc)+(double)pow(static_cast<double>(k),-1))*(double)pow(static_cast<double>(a),2))/k;
			
		std::cout << "variance: " << variance(acc) << endl;
		std::cout << "d2: " << pow(d,2) << '\n';
		std::cout << "d_min: " << d_min << '\n';
		std::cout << "avg in [" << mean(acc) - sqrt(d_min) << "," << mean(acc) + sqrt(d_min) << "]" << endl; 

		g_index++;
		}	
	}
	if(!child) {
		double min_v = boost::accumulators::extract::min(acc);
		double max_v = boost::accumulators::extract::max(acc);

		std::cout << "EXITING" << endl;
		std::cout << "Number of Simulations: " << k << '\n';
		std::cout << "Average Simuation Time: " << mean(acc) << '\n';
		std::cout << "Max Simuation Time: " << max_v << '\n';
		std::cout << "Min Simuation Time: " <<  min_v << '\n';
		std::cout << "Variance: " << variance(acc) << '\n';

		ofstream f_grouped_data;
		f_grouped_data.open("data_g.dat");

		double t;
		for(t = floor(min_v); t <= floor(max_v); t++) {
			std::cout << t << " " << classes.count(t) << endl;
			f_grouped_data << t << " " << (double) classes.count(t)/k << endl;
		}

		f_grouped_data.close();

		plot(ceil(max_v)-floor(min_v), floor(min_v), ceil(max_v), mean(acc), variance(acc));

		std::string const command = "gnuplot plot";
		system(command.c_str());
	}
	return 0;
}

