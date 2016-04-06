#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include "estimation.hpp"
#include "utility.hpp"

using namespace std;
using namespace boost::accumulators;
#define BUFSIZE 1024
#define BATCHSIZE 8

namespace chowrobbins {

typedef boost::unordered_map<std::string, double> map_b;
typedef boost::unordered_map<std::string, Estimation> map_e;

bool cAndRTest(boost::unordered_map<std::string, Estimation> formulae, boost::unordered_map<std::string, double> map_d_est) {
	bool result = true;
	BOOST_FOREACH(map_e::value_type i, formulae) {
		if(!i.second.is_concluded()) {
			if(map_d_est.count(i.first) == 1) {	
				bool test = map_d_est.at(i.first) <= pow(i.second.get_confidence_interval(),2);	
				if(test)
					i.second.set_concluded(1);
			}
		}
		result = result && i.second.is_concluded();
	}
	return result;
}

int evaluate(std::string model, std::string platform_file, 
								std::string deployment_file, boost::unordered_map<std::string, Estimation> formulae) {

	std::cout << "-- Running Chow and Robbins --" << endl;

	int k = 0; /* keeps track of the number of dispatched processes */	
	int child = 0; /* boolean to identify a child */
	int child_idx = -1; /* child's index */
	int group_idx = 0; /* number of groups */
	int group_ctr; /* a counter to keep track of the number of processes added to a group */
	int fd[BATCHSIZE][2]; /* array with a file descriptor for each process */
	pid_t pid[BATCHSIZE]; /* array with the pid of each process */

	std::cout << "PLATFORM FILE: " << std::string(platform_file) << endl;
	std::cout << "DEPLOYMENT FILE: " << std::string(deployment_file)  << endl;
	std::cout << "MODEL: " << std::string(model) << endl;

	std::vector<std::string> keys;
	std::vector<accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > accs;
	
	boost::unordered_map<std::string, double> map_a;
	multiset<std::string> vars;

	BOOST_FOREACH(map_e::value_type i, formulae) {
		double error = 1.0-i.second.get_coverage_probability();
		map_a[i.first] = boost::math::erf_inv(1.0-error/2.0);
		vars.insert(i.second.get_variable());
	}

	boost::unordered_map<std::string, double> map_d_est;
	boost::unordered_map<std::string, multiset<double> > map_classes;
	boost::unordered_map<std::string, accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, 
																	   tag::median, tag::sum> > > map_accs;
	char * temp;
	char * token;
	char * sequence;

	while(!(cAndRTest(formulae,map_d_est)) || keys.empty()) {	
		for(group_ctr = 0; (group_ctr < BATCHSIZE) && (!(cAndRTest(formulae,map_d_est)) || keys.empty()); group_ctr++) { 
			// generates a group of children processes 	
			pipe(fd[k%BATCHSIZE]);
			pid[k%BATCHSIZE] = ::fork();
			if(pid[k%BATCHSIZE] == 0) {
				child_idx = k%BATCHSIZE;
				child = 1;
				break; // child process gets out of the for loop 
			}
			k++;			
		}
		if(child) { // child process calls the code to be simulated and dies 
			close(fd[child_idx][0]);

			// std::string const command = std::string("java chord/Chord chord/platform.xml chord/chord.xml | ./pipereader")
			
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
			for (i = group_idx * BATCHSIZE; i < k; i++) {
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
        				for(j = 0; j < position/2; j++) { /* previously <= (position-1)/2 */
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
		BOOST_FOREACH(map_e::value_type i, formulae) {
			if(!i.second.is_concluded()) {
				std::string var = i.second.get_variable();
				map_d_est[i.first] = ((variance(map_accs.at(var))+
									  (double)pow(static_cast<double>(k),-1))*
									  (double)pow(static_cast<double>(map_a[i.first]),2))/k;	
			}
		}
		std::cout << "\r" << k << flush;
		group_idx++;
		}	
	}
	if(!child) {
		int l;

		std::cout << endl << "Number of Simulations: " << k << endl;

		ofstream f_grouped_data[2];

		for(l = 0; l < keys.size(); l++) {
			if(vars.count(keys[l]) == 1) {
				std::cout << "--- " << keys[l] << " ---" << std::endl;

				double min_v = boost::accumulators::extract::min(map_accs.at(keys[l]));
				double max_v = boost::accumulators::extract::max(map_accs.at(keys[l]));

				std::cout << "Mean: " << mean(map_accs.at(keys[l])) << '\n';
				std::cout << "Max: " << max_v << '\n';
				std::cout << "Min: " <<  min_v << '\n';
				std::cout << "Variance: " << variance(map_accs.at(keys[l])) << '\n';
			
				f_grouped_data[l].open(std::string(std::string("data-") + std::string(keys[l]) + std::string(".dat")).c_str());

				double m;
				for(m = floor(min_v); m <= floor(max_v); m++) {
					std::cout << m << " " << map_classes[keys[l]].count(m) << endl;
					f_grouped_data[l] << m << " " << (double) map_classes[keys[l]].count(m)/k << endl;
				}

				utility::plot(ceil(max_v)-floor(min_v), floor(min_v), ceil(max_v), mean(map_accs.at(keys[l])), variance(map_accs.at(keys[l])), keys[l]);

				std::string const command = std::string("gnuplot ") + std::string("plot_") + keys[l];
				system(command.c_str());

				f_grouped_data[l].close();
			}
		}

		BOOST_FOREACH(map_e::value_type i, formulae) {

			std::cout << "--- " << i.first << " ---" << std::endl;
			std::string var = i.second.get_variable(); 
			std::cout << var << ": " << "[" << mean(map_accs.at(var)) - sqrt(map_d_est.at(i.first)) << ","  
											<< mean(map_accs.at(var)) + sqrt(map_d_est.at(i.first)) << "]" << endl;
		}

	}
	return 0;
}
}