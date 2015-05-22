#include <iostream>
using std::cout; using std::endl;
#include <fstream>
#include <string>
#include <cmath>
#include <boost/lexical_cast.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/math/distributions/binomial.hpp>
#include "utility.hpp"

/*
#include <iomanip>
using std::fixed; using std::left; using std::right; using std::right; using std::setw;
using std::setprecision;
*/

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
	f_plot << "hist(x,width)=width*floor(x/width)+width/2.0" << endl;
	f_plot << "set xrange [min:max]" << endl;
	f_plot << "set yrange [0:]" << endl;
	//#to put an empty boundary around the
	//#data inside an autoscaled graph.
	f_plot << "set offset graph 0.05,0.05,0.05,0.0" << endl;
	f_plot << "set xtics min,(max-min)/5,max" << endl;
	f_plot << "set boxwidth width*0.9" << endl;
	f_plot << "set style fill solid 0.5" << endl; // #fillstyle 
	f_plot << "set tics out nomirror" << endl;
	f_plot << "set xlabel " << "\"Avg\"" << endl;
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

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */
	int runs = atoi(argv[4]); /* total number of process to launch */

	/*
	double p0 = atof(argv[5]);
	double p1 = atof(argv[6]);
	double alpha = atof(argv[7]);
	double beta = atof(argv[8]);
	*/

	/*
	std::cout << "Enter a value for p0" << endl;
	std::cin >> p0;
	std::cout << "Enter a value for p1" << endl;
	std::cin >> p1;
	std::cout << "Enter a value for alpha" << endl;
	std::cin >> alpha;
	std::cout << "Enter a value for beta" << endl;
	std::cin >> beta;
	*/

	double data[runs]; /* array with the outcome of each process */
	
	pid_t pid[BATCHSIZE]; /* array with the pid of each process */
	
	int fd[BATCHSIZE][2]; /* array with a file descriptor for each process */

	int child = 0; /* boolean to identify a child */
	int child_idx = -1; /* child's index */

	int g_index = 0; /* number of groups */
	int g_counter; /* a counter to keep track of the number of processes added to a group */

	std::cout << "N RUNS: " << boost::lexical_cast<string>(runs) << endl;
	std::cout << "PLATFORM FILE: " << std::string(platform_file) << endl;
	std::cout << "DEPLOYMENT FILE: " << std::string(deployment_file)  << endl;
	std::cout << "MODEL: " << std::string(model) << endl;

	int k = 0; /* keeps track of the number of dispatched processes */

	/* Sequential Acceptance Sampling */

	int n_below = 0;
	int n_above = 0;

	double n_target = 14.0;

	double p0 = 0.9;
	double p1 = 0.7;
	double alpha = 0.2;
	double beta = 0.1;

	pair<double,double> sampling_plan = utility::singleSamplingPlan(p0,p1,alpha,beta);

	double n_A = (1.0 - beta)/(alpha);
	double n_B = (beta)/(1 - alpha);

	double sprt_fm = 0.0;

	using namespace boost::accumulators;

	accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > acc;

	ofstream f_data;

	f_data.open("data.dat");

	while(k < runs) {
		if(!child) {
			for(g_counter = 0; (g_counter < BATCHSIZE && k < runs); g_counter++) { /* generates a group of children processes */	
				pipe(fd[k%BATCHSIZE]);
				pid[k%BATCHSIZE] = ::fork();
				if(pid[k%BATCHSIZE] == 0) {
					child_idx = k%BATCHSIZE;
					child = 1;
					break; /* child process gets out of the for loop */
				}
				k++;			
			}
		}
		if(child) { /* child process calls the code to be simulated and dies */
			//std::cout << "CHILD: " << child_idx << " FINISHING" << endl;
			close(fd[child_idx][0]);
			std::string const command = "./" + std::string(model) + " " + std::string(platform_file)  
											 + " "  + std::string(deployment_file) 
											 + " " + boost::lexical_cast<string>(fd[child_idx][1])
											 + " " + boost::lexical_cast<string>(k);
			system(command.c_str());
			close(fd[child_idx][1]);
			break; 
		}
		if(!child) { /* treats the data of each group */
			wait(NULL);
			//std::cout << "PRINTING GROUP: " << g_index << endl;
			int i;
			accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median> > acc_b;

			for (i = g_index * BATCHSIZE; i < k; i++) {
	 			char buffer[BUFSIZE] = "";
				waitpid(pid[i%BATCHSIZE], NULL, 0);
				close(fd[i%BATCHSIZE][1]);
				read(fd[i%BATCHSIZE][0], &buffer, sizeof(buffer));
				double v = atof(buffer);
				data[i] = v;
				acc(v);
				acc_b(v);
				f_data << v << endl;

				if(v < n_target) {
					n_below++;
				}
				else {
					n_above++;
				}	
				//std::cout << "In " << i << " " << data[i] << endl;
				close(fd[i%BATCHSIZE][0]);	
			}

			/*
				k : keeps track of the total number of samples so far (m)
				n_above/n_below : keeps track of the number of successes so far depending on the critera (dm)
			*/

		if(k >= sampling_plan.first) {
			cout << "-- Simple Sequential Test --" << endl;	
			if(n_above > sampling_plan.second) {
				cout << "SST: We fail to reject H0" << endl;
				cout << "n: " << sampling_plan.first << endl;
				cout << "c: " << sampling_plan.second << endl;
				cout << "n_above: " << n_above << endl;
				cout << "alpha: " << alpha << endl;
				cout << "beta: " << beta << endl;
				cout << "k: " << k << endl;
				cout << "H0: " << "p >= " << p0 << endl;
				cout << "H1: " << "p <= " << p1 << endl;
				cout << "Target average: " << n_target << endl;
			}
			else {
				cout << "SST: H0 is rejected" << endl;
				cout << "n: " << sampling_plan.first << endl;
				cout << "c: " << sampling_plan.second << endl;
				cout << "n_above: " << n_above << endl;
				cout << "alpha: " << alpha << endl;
				cout << "beta: " << beta << endl;
				cout << "k: " << k << endl;
				cout << "H0: " << "p >= " << p0 << endl;
				cout << "H1: " << "p <= " << p1 << endl;
				cout << "Target average: " << n_target << endl;
			}
		}

		cout << "-- Sequential Probability Ratio Test --" << endl;
		double n_fm = utility::fm(p0,p1,k,n_above);

		if(n_fm >= n_A && k > sampling_plan.first && n_above > sampling_plan.second) {
			cout << "SPRT1: Condition A is verified, H0 is rejected" << endl;
			cout << "n: " << sampling_plan.first << endl;
			cout << "c: " << sampling_plan.second << endl;
			cout << "alpha: " << alpha << endl;
			cout << "beta: " << beta << endl;
			cout << "fm: " << n_fm << endl;
			cout << "k: " << k << endl;
			cout << "H0: " << "p >= " << p0 << endl;
			cout << "H1: " << "p <= " << p1 << endl;
			cout << "Target average: " << n_target << endl;
		}
		else if(n_fm <= n_B && k > sampling_plan.first && n_above > sampling_plan.second) {
			cout << "SPRT1: Condition B is verified, we fail to reject H0" << endl;
			cout << "n: " << sampling_plan.first << endl;
			cout << "c: " << sampling_plan.second << endl;
			cout << "alpha: " << alpha << endl;
			cout << "beta: " << beta << endl;
			cout << "fm: " << n_fm << endl;
			cout << "k: " << k << endl;
			cout << "H0: " << "p >= " << p0 << endl;
			cout << "H1: " << "p <= " << p1 << endl;
			cout << "Target average: " << n_target << endl;
		}
		else {
			cout << "None of the conditions are verified thus far" << endl;
		}

		//sprt_fm = n_below*log(p1/p0)+n_above*log((1-p1)/(1-p0)); // dm and n-dm respectively 
		sprt_fm = n_above*log(p1/p0)+n_below*log((1-p1)/(1-p0)); // dm and n-dm respectively 

		if(sprt_fm <= log(beta/(1-alpha)) && !((log(beta/(1-alpha)) < sprt_fm) && (sprt_fm < log((1-beta)/alpha)))) {
			cout << "SPRT2: We fail to reject H0" << endl;
			cout << "n: " << sampling_plan.first << endl;
			cout << "c: " << sampling_plan.second << endl;
			cout << "alpha: " << alpha << endl;
			cout << "beta: " << beta << endl;
			cout << "fm: " << n_fm << endl;
			cout << "k: " << k << endl;
			cout << "H0: " << "p >= " << p0 << endl;
			cout << "H1: " << "p <= " << p1 << endl;
			cout << "Target average :" << n_target << endl;
		}
		else if(!((log(beta/(1-alpha)) < sprt_fm) && (sprt_fm < log((1-beta)/alpha)))) {
			cout << "SPRT2: H0 is rejected" << endl;
			cout << "n: " << sampling_plan.first << endl;
			cout << "c: " << sampling_plan.second << endl;
			cout << "alpha: " << alpha << endl;
			cout << "beta: " << beta << endl;
			cout << "fm: " << n_fm << endl;
			cout << "k: " << k << endl;
			cout << "H0: " << "p >= " << p0 << endl;
			cout << "H1: " << "p <= " << p1 << endl;
			cout << "Target average :" << n_target << endl;
		}

		g_index++;
		}
	}
	if(!child) {
		std::cout << "EXITING" << endl;
		std::cout << "Number of Simulations: " << runs << '\n';
		std::cout << "Average Simuation Time: " << mean(acc) << '\n';
		std::cout << "Max Simuation Time: " << boost::accumulators::extract::max(acc) << '\n';
		std::cout << "Min Simuation Time: " <<  boost::accumulators::extract::min(acc) << '\n';
		std::cout << "Variance: " << variance(acc) << '\n';

		std::cout << "% of values above: " << ((double) n_above/(double) runs) * 100 << endl;
		std::cout << "% of values below: " << ((double) n_below/(double) runs) * 100 << endl;

		std::cout << "A: " << n_A << endl;
		std::cout << "B: " << n_B << endl;

		double min_v = boost::accumulators::extract::min(acc);
		double max_v = boost::accumulators::extract::max(acc);
		int diff = ceil(max_v)-floor(min_v);

		std::vector<double> lims(diff);

		int m;
		for(m = 0; m < lims.size(); m++) {
			lims[m] = floor(min_v)+1.0*m;
		}

		std::vector<int> grps(diff,0);

		int n;
		for(m = 0; m < runs; m++) {
			for(n = 0; n < lims.size(); n++) {
				if(data[m] >= lims[n] && data[m] < lims[n]+1.0) {
					grps[n]++;
				}
			}
		}

		std::cout << "--------" << endl;

		for(m = 0; m < grps.size(); m++) {
			int l;
			for(l = 0; l < grps[m]; l++) {
				cout << "*";
			}
			cout << endl;
		}

		ofstream f_grouped_data;
		f_grouped_data.open("data_g.dat");

		for(m = 0; m < grps.size(); m++) {
			f_grouped_data << lims[m] << " " << (double) grps[m]/runs << endl;
		}

		f_grouped_data.close();
		f_data.close();

		plot(diff, floor(min_v), ceil(max_v), mean(acc), variance(acc));

		std::string const command = "gnuplot plot";
		system(command.c_str());

	}
	return 0;
}