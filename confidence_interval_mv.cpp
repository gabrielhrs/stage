#include <iostream>
using std::cout; using std::endl;
#include <fstream>
#include <string>
#include <cmath>
#include <set>
#include <iterator>
#include <boost/lexical_cast.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include "utility.hpp"

using namespace std;
#define BUFSIZE 1024
#define BATCHSIZE 8

typedef boost::unordered_map<std::string, double> map_b;

void plot(int n_int, int n_min, int n_max, double u, double s, std::string n_var) {

	ofstream f_plot;
	f_plot.open(std::string(std::string("plot_") + std::string(n_var)).c_str());

	f_plot << "reset" << endl;

	f_plot << "n = " << n_int << endl; // #number of intervals
	f_plot << "max = " << n_max << endl; //. #max value
	f_plot << "min = " << n_min << endl; // #min value
	f_plot << "width = (max-min)/n" << endl; // #interval width
	//function used to map a value to the intervals
	f_plot << "set xrange [min:max]" << endl;
	f_plot << "set yrange [0:]" << endl;
	//to put an empty boundary around the data inside an autoscaled graph.
	f_plot << "set offset graph 0.05,0.05,0.05,0.0" << endl;
	f_plot << "set xtics 1" << endl; //min,(max-min)/5,max" << endl;
	f_plot << "set boxwidth width*0.9" << endl;
	f_plot << "set style fill solid 0.5" << endl; // #fillstyle 
	f_plot << "set tics out nomirror" << endl;
	f_plot << "set xlabel " << "\"" + n_var + "\"" << endl;
	f_plot << "set ylabel " << "\"Relative Frequency\"" << endl;
	//gauss
	f_plot << "s = " << s << endl;
	f_plot << "u = " << u << endl;
	f_plot << "gauss(x) = (1/(sqrt(2*s*pi)))*exp((-(x-u)**2)/(2*s))" << endl;
	//count and plot
	f_plot << "plot \"data_g_" + n_var + ".dat\" w boxes, gauss(x)" << endl;
	f_plot.close();
}

bool cITest(boost::unordered_map<std::string, double> map_w_est, double w) {
	bool result = false;
	BOOST_FOREACH(map_b::value_type i, map_w_est) {
		result = result || i.second > w;
	}
	return result;
}

int main(int argc, char *argv[]) {

	std::cout << "-- Running Confidence Interval --" << endl;

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */

	int child = 0; /* boolean to identify a child */
	int child_idx = -1; /* child's index */
	int g_index = 0; /* number of groups */
	int g_counter; /* a counter to keep track of the number of processes added to a group */
	int fd[BATCHSIZE][2]; /* array with a file descriptor for each process */
	pid_t pid[BATCHSIZE]; /* array with the pid of each process */

	std::cout << "PLATFORM FILE: " << std::string(platform_file) << endl;
	std::cout << "DEPLOYMENT FILE: " << std::string(deployment_file)  << endl;
	std::cout << "MODEL: " << std::string(model) << endl;

	int k = 0; /* keeps track of the number of dispatched processes */

	using namespace boost::accumulators;

	double s2;
	double w = atof(argv[5]);
	double conf_level = 1 - atof(argv[4]);
	

	std::vector<std::string> keys;
	std::vector<accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > accs;
	
	boost::unordered_map<std::string, double> map_w_est;
	boost::unordered_map<std::string, multiset<double> > map_classes;
	boost::unordered_map<std::string, accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > map_accs;

	char * temp;
	char * token;
	char * sequence;

	std::multiset<double> classes;

	while(cITest(map_w_est, w) || keys.empty()) {
		if(!child) {
			for(g_counter = 0; (g_counter < BATCHSIZE) && (cITest(map_w_est, w) || keys.empty()); g_counter++) { // generates a group of children processes 		
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
			for (i = g_index * BATCHSIZE; i < k; i++){
	 			char buffer[BUFSIZE] = "";
				waitpid(pid[i%BATCHSIZE], NULL, 0);
				close(fd[i%BATCHSIZE][1]);
				read(fd[i%BATCHSIZE][0], &buffer, sizeof(buffer));
				close(fd[i%BATCHSIZE][0]);	
				sequence = strdup(buffer);
				int position = 0;
				if(sequence != NULL) {
					temp = sequence;
					if(keys.empty()) {
						while((token = strsep(&sequence, ",")) != NULL) {
							if(position%2 == 0) {
			                	keys.push_back(boost::lexical_cast<string>(token));
							}
							else {
                				accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > acc;
                				acc(boost::lexical_cast<double>(token));   
                				accs.push_back(acc);
							}
							position++;
						}
						free(temp);
						int j;
        				for(j = 0; j <= (position-1)/2; j++) {
            				map_accs[keys[j]] = accs[j];
            				std::multiset<double> classes;
            				classes.insert(floor(boost::accumulators::extract::min(accs[j])));
            				map_classes[keys[j]] = classes;
        				}
					}
					else {
						std::string key;
						while((token = strsep(&sequence, ",")) != NULL) {
							if(position%2 == 0) {
			                	key = boost::lexical_cast<string>(token);
							}
							else {
                				map_accs.at(key)(boost::lexical_cast<double>(token));
                				map_classes.at(key).insert(floor(boost::lexical_cast<double>(token)));
							}
							position++;
						}
						free(temp);
					}
				}				
			}

		int l;
		for(l = 0; l < keys.size(); l++) {
			s2 = ((k)/(k-1))*variance(map_accs.at(keys[l])); 		// unbiased estimator
			boost::math::students_t dist(k-1);
			map_w_est[keys[l]] = (boost::math::quantile(complement(dist,conf_level/2))*s2/sqrt(k));
		}
		std::cout << "\r" << k << flush;		
		g_index++;
		}	
	}
	if(!child) {
		int l;

		std::cout << endl << "Number of Simulations: " << k << endl;

		ofstream f_grouped_data[2];

		for(l = 0; l < keys.size(); l++) {
		
			std::cout << "--- " << keys[l] << " ---" << std::endl;

			double min_v = boost::accumulators::extract::min(map_accs.at(keys[l]));
			double max_v = boost::accumulators::extract::max(map_accs.at(keys[l]));

			std::cout << "Mean: " << mean(map_accs.at(keys[l])) << '\n';
			std::cout << "Max: " << max_v << '\n';
			std::cout << "Min: " <<  min_v << '\n';
			std::cout << "Variance: " << variance(map_accs.at(keys[l])) << '\n';
		
			f_grouped_data[l].open(std::string(std::string("data_g_") + std::string(keys[l]) + std::string(".dat")).c_str());

			double t;
			for(t = floor(min_v); t <= floor(max_v); t++) {
				std::cout << t << " " << map_classes[keys[l]].count(t) << endl;
				f_grouped_data[l] << t << " " << (double) map_classes[keys[l]].count(t)/k << endl;
			}

			plot(ceil(max_v)-floor(min_v), floor(min_v), ceil(max_v), mean(map_accs.at(keys[l])), variance(map_accs.at(keys[l])), keys[l]);

			std::string const command = std::string("gnuplot ") + std::string("plot_") + keys[l];
			system(command.c_str());

			f_grouped_data[l].close();

			std::cout << keys[l] << ": " << mean(map_accs.at(keys[l]))-map_w_est[keys[l]] << "," 
										 << mean(map_accs.at(keys[l]))+map_w_est[keys[l]] << "]" << endl;	

		}
	}
	return 0;
}