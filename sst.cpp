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

typedef boost::unordered_map<std::string, Hypothesis> map_h;
typedef boost::unordered_map<std::string, double> map_b;

namespace sst {

	bool sstTest(boost::unordered_map<std::string, Hypothesis> hypotheses, boost::unordered_map<std::string, pair<double,double> > map_sp,
				 boost::unordered_map<std::string, double> map_dm, int m) {
		bool result = false;
		BOOST_FOREACH(map_h::value_type i, hypotheses) {
			if(map_dm.count(i.first) == 1 && map_sp.count(i.first) == 1) {
				std::cout << "dm: " << map_dm.at(i.first)
						  << " c: " << map_sp.at(i.first).second 
						  << " n: " << map_sp.at(i.first).first
						  << " m: " << m << std::endl;
				result = result || ((map_dm.at(i.first) <= map_sp.at(i.first).second) && (map_dm.at(i.first) + map_sp.at(i.first).first - m > map_sp.at(i.first).second));  	
				std::cout << result << std::endl;
			}
		}
	}

	int sst(std::string model, std::string platform_file, std::string deployment_file, 
														   boost::unordered_map<std::string, Hypothesis> hypotheses) {

		std::cout << "-- Running SST --" << endl;

		int m = 0; /* keeps track of the number of dispatched processes */

		std::vector<std::string> keys;
		std::vector<accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > accs;
		boost::unordered_map<std::string, double> map_dm;
		boost::unordered_map<std::string, double> map_fm;
		boost::unordered_map<std::string, multiset<double> > map_classes;
		boost::unordered_map<std::string, pair<double,double> > map_sp;
		boost::unordered_map<std::string, accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, 
																		   tag::median, tag::sum> > > map_accs;

		char * temp;
		char * token;
		char * sequence;
		
		BOOST_FOREACH(map_h::value_type i, hypotheses) {
			map_dm[i.first] = 0;
			map_sp[i.first] = utility::singleSamplingPlan(i.second.get_p0(), i.second.get_p1(), i.second.get_alpha(), i.second.get_beta());		
			std::cout << "n: " << map_sp.at(i.first).first << " c: " << map_sp.at(i.first).second << std::endl;
		}

		pid_t pid[BATCHSIZE]; 
		int fd[BATCHSIZE][2];
		int child = 0; /* boolean to identify a child */ 
		int child_idx = -1; /* child's index */
		int group_idx = 0; // CHECK THE NEED OF THIS!
		int group_ctr; /* a counter to keep track of the number of processes added to a group */

		while(!sstTest(hypotheses,map_sp,map_dm,m) || keys.empty()) {
		//while(sstTest(map_dm,map_sp,m) || keys.empty()) {
			if(!child) {
				//for(group_ctr = 0; (group_ctr < BATCHSIZE && (sstTest(map_dm,map_sp,m) || keys.empty())); group_ctr++) { /* generates a group of children processes */	
				for(group_ctr = 0; (group_ctr < BATCHSIZE && (!sstTest(hypotheses,map_sp,map_dm,m) || keys.empty())); group_ctr++) { 
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
	            				/*
								if(map_func[keys[j]](boost::accumulators::extract::min(accs[j]), map_h_values[keys[j]])) {
									map_dm[keys[j]]++;	
								}
								map_fm[keys[j]] = map_dm[keys[j]]*log(p1/p0)+(m-map_dm[keys[j]])*log((1-p1)/(1-p0));
	       						*/
	       					}
							BOOST_FOREACH(map_h::value_type i, hypotheses) {
								if(i.second.get_fH0()(boost::accumulators::extract::min(map_accs.at(i.second.get_variable())), i.second.get_v1())) {
									map_dm[i.first]++;
								}
								/*
								map_fm[i.first] = map_dm.at(i.first)*log(i.second.get_p1()/i.second.get_p0())+(m-map_dm.at(i.first))
																    *log((1-i.second.get_p1())/(1-i.second.get_p0()));
								*/
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

	               					/* 	CHECK THIS ON EVERY FILE */	

									BOOST_FOREACH(map_h::value_type i, hypotheses) {
										if(i.second.get_variable() == key) {
											if(i.second.get_fH0()(boost::lexical_cast<double>(token), i.second.get_v1())) {
												map_dm[i.first]++;
											}
										}
										/*
										map_fm[i.first] = map_dm.at(i.first)*log(i.second.get_p1()/i.second.get_p0())+(m-map_dm.at(i.first))
																 		 *log((1-i.second.get_p1())/(1-i.second.get_p0()));
										*/
									}
	               					
									/*
									if(map_func[key](boost::lexical_cast<double>(token), map_h_values[key])) {
										map_dm[key]++;	
									}
									map_fm[key] = map_dm[key]*log(p1/p0)+(m-map_dm[key])*log((1-p1)/(1-p0));
									*/
								}
								position++;
							}
							free(temp);
						}
					}		
				}
			}
			/*
			BOOST_FOREACH(map_b::value_type i, map_dm){
				std::cout << "m: " << m << " dm: " << i.second << std::endl;
			}
			*/
			group_idx++;
			//std::cout << "\r" << m << flush;
		}
		if(!child) {
			BOOST_FOREACH(map_h::value_type i, hypotheses) {
				//if(map_fm[i.first] <= log(i.second.get_beta()/(1-i.second.get_alpha()))) {
				if(map_dm.at(i.first) > map_sp.at(i.first).second) {				
						std::cout << "Runs: " << m << std::endl;
						std::cout << "Fail to reject H0 for " << i.first << "." << endl;
				}
				else {
						std::cout << "Runs: " << m << endl;
						std::cout << "H0 is rejected. H1 is accepted for " << i.first << "." << endl;
				}
			}
			/*
			int l;
			for(l = 0; l < keys.size(); l++) {
				if(map_fm[keys[l]] <= log(beta/(1-alpha))) {
					std::cout << "Runs: " << m << endl;
					std::cout << "Fail to reject H0 for " << l << "." << endl;
				}
				else {
					std::cout << "Runs: " << m << endl;
					std::cout << "H0 is rejected. H1 is accepted for " << l << "." << endl;
				}
			}
			*/
		}

		return 0;
	}
}