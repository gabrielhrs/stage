#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include "hypothesis.hpp"
#include "utility.hpp"

using namespace std;
using namespace boost::accumulators;
#define BUFSIZE 1024
#define BATCHSIZE 8

typedef boost::unordered_map<std::string, double> map_b;
typedef boost::unordered_map<std::string, Hypothesis> map_h;

namespace srpt {

	bool srptTest(boost::unordered_map<std::string, Hypothesis> hypotheses, boost::unordered_map<std::string, double> map_fm) {
		bool result = false;
		BOOST_FOREACH(map_h::value_type i, hypotheses) {
		/*
		*/
		if(map_fm.count(i.first) == 1)
			result = result || (log(i.second.get_beta()/(1-i.second.get_alpha())) < map_fm.at(i.first) 
								&& map_fm.at(i.first) < log((1-i.second.get_beta())/i.second.get_alpha()));;
		
		}
		return result;
	}

	int srpt(std::string model, std::string platform_file, std::string deployment_file, 
														   boost::unordered_map<std::string, Hypothesis> hypotheses) {

		std::cout << "-- Running SRPT --" << endl;

		int m = 0; /* keeps track of the number of dispatched processes */
		int child = 0; /* boolean to identify a child */
		int child_idx = -1; /* child's index */
		int group_idx = 0; /* number of groups */
		int group_ctr; /* a counter to keep track of the number of processes added to a group */ 
		int fd[BATCHSIZE][2]; /* array with a file descriptor for each process */ 
		pid_t pid[BATCHSIZE]; /* array with the pid of each process */ 

		std::vector<std::string> keys;
		std::vector<accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > accs;
		
		boost::unordered_map<std::string, int> map_dm;
		boost::unordered_map<std::string, double> map_fm;
		boost::unordered_map<std::string, multiset<double> > map_classes;
		boost::unordered_map<std::string, accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, 
																		   tag::median, tag::sum> > > map_accs;

		char * temp;
		char * token;
		char * sequence;

		BOOST_FOREACH(map_h::value_type i, hypotheses) {
			map_dm[i.first] = 0;
		}
		
		while(srptTest(hypotheses, map_fm) || keys.empty()) {
			if(!child) {
				for(group_ctr = 0; (group_ctr < BATCHSIZE && (srptTest(hypotheses, map_fm) || keys.empty())); group_ctr++) { 
					/* generates a group of children processes */	
					pipe(fd[m%BATCHSIZE]);
					pid[m%BATCHSIZE] = ::fork();
					if(pid[m%BATCHSIZE] == 0) {
						child_idx = m%BATCHSIZE;
						child = 1;
						break; /* child process gets out of the for loop */
					}
					m++;			
				}
			}
			if(child) { /* child process calls the code to be simulated and dies */
				close(fd[child_idx][0]);
				std::string const command = "./" + std::string(model) + " " + std::string(platform_file)  
												 + " "  + std::string(deployment_file) 
												 + " " + boost::lexical_cast<string>(fd[child_idx][1])
												 + " " + boost::lexical_cast<string>(m);
				system(command.c_str());
				close(fd[child_idx][1]);
				break; 
			}
			if(!child) { /* treats the data of each group */
				wait(NULL);
				int i;
				for (i = group_idx * BATCHSIZE; i < m; i++) {
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
                					accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, 
                													 tag::median, tag::sum> > acc;
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
        					BOOST_FOREACH(map_h::value_type i, hypotheses) {
								
							if(i.second.get_fH0()(boost::accumulators::extract::min(map_accs.at(i.second.get_variable())), 
														  i.second.get_v1())) {
								map_dm[i.first]++;	
							}
							map_fm[i.first] = map_dm[i.first]*log(i.second.get_p1()/i.second.get_p0())+
							                  (m-map_dm[i.first])*log((1-i.second.get_p1())/(1-i.second.get_p0()));										

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
									BOOST_FOREACH(map_h::value_type i, hypotheses) {
										if(i.second.get_variable() == key) { // CHECK THIS!!!!!!
											if(i.second.get_fH0()(boost::accumulators::extract::min(map_accs.at(i.second.get_variable())), 
																  i.second.get_v1())) {
												map_dm[i.first]++;	
											}
											map_fm[i.first] = map_dm[i.first]*log(i.second.get_p1()/i.second.get_p0())+
														      (m-map_dm[i.first])*log((1-i.second.get_p1())/(1-i.second.get_p0()));										
										}
									}
								}
								position++;
							}
							free(temp);
						}
					}		
				}
			}
			std::cout << "\r" << m << flush;		
		}
		if(!child) {
			std::cout << std::endl;
			std::cout << "Number of simulations: " << m << std::endl;
			BOOST_FOREACH(map_h::value_type i, hypotheses) {
				if(map_fm[i.first] <= log(i.second.get_beta()/(1-i.second.get_alpha()))) {
					std::cout << "--- " << i.first << " ---" << std::endl;
					std::cout << "Fail to reject H0 for " << i.first << "." << endl;
				}
				else {
					std::cout << "--- " << i.first << " ---" << std::endl;
					std::cout << "H0 is rejected. H1 is accepted for " << i.first << "." << endl;
				}
			}
		}
		return 0;
	}
}