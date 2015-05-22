#include <iostream>
using std::cout; using std::endl;
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
#include "utility.hpp"

using namespace std;
#define BUFSIZE 1024
#define BATCHSIZE 8

typedef boost::unordered_map<std::string, double> map_b;

bool greaterThanXY(double x, double y) {
	return x > y;
}

bool smallerThanXY(double x, double y) {
	return x < y;
}

bool srptTest(boost::unordered_map<std::string, double> map_fm, double p0, double p1, double alpha, double beta) {
	bool result = false;
	BOOST_FOREACH(map_b::value_type i, map_fm) {
		result = result || (log(beta/(1-alpha)) < i.second && i.second < log((1-beta)/alpha));
	}	
	return result;
}

int main(int argc, char *argv[]) {

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */
	
	double p0 = 0.9;
	double p1 = 0.85;
	double alpha = 0.2;
	double beta = 0.1;

	if(p0 == 1 || p1 == 0) {
		/*
		std::string const command = "./sst" + std::string(model) + " " + std::string(platform_file)  
										 + " "  + std::string(deployment_file) 
										 + " " + boost::lexical_cast<string>(v_test);
		system(command.c_str());
		*/
	}
	else {

		std::cout << "-- Running SRPT --" << endl;

		int m = 0;
		int child = 0; 
		int child_idx = -1; 
		int g_index = 0;
		int g_counter; 
		int fd[BATCHSIZE][2]; 
		pid_t pid[BATCHSIZE]; 

		using namespace boost::accumulators;

		std::vector<std::string> keys;
		std::vector<accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > accs;

		boost::unordered_map<std::string, int> map_dm;
		boost::unordered_map<std::string, double> map_fm;
		boost::unordered_map<std::string, double> map_h_values;
		boost::unordered_map<std::string, bool(*)(double,double)> map_func;
		boost::unordered_map<std::string, multiset<double> > map_classes;
		boost::unordered_map<std::string, accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > > map_accs;

		boost::unordered_map<std::string, double> map_alpha;
		boost::unordered_map<std::string, double> map_beta;
		boost::unordered_map<std::string, double> map_p0;
		boost::unordered_map<std::string, double> map_p1;

		char * temp;
		char * token;
		char * sequence;

		/* -- Setting Values -- */
		bool (*f1)(double,double);
		f1 = &greaterThanXY;
		map_func["tt"] = map_func["dv"] = f1;
		map_h_values["tt"] = 11;
		map_h_values["dv"] = 1;
		//
		//
		/* -------------------- */
		
		while(srptTest(map_fm,p0,p1,alpha,beta) || keys.empty()) {
			if(!child) {
				for(g_counter = 0; (g_counter < BATCHSIZE && (srptTest(map_fm,p0,p1,alpha,beta) || keys.empty())); g_counter++) { /* generates a group of children processes */	
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
				for (i = g_index * BATCHSIZE; i < m; i++) {
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

								if(map_func[keys[j]](boost::accumulators::extract::min(accs[j]), map_h_values[keys[j]])) {
									map_dm[keys[j]]++;	
								}
								map_fm[keys[j]] = map_dm[keys[j]]*log(p1/p0)+(m-map_dm[keys[j]])*log((1-p1)/(1-p0));
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

									if(map_func[key](boost::lexical_cast<double>(token), map_h_values[key])) {
										map_dm[key]++;	
									}
									map_fm[key] = map_dm[key]*log(p1/p0)+(m-map_dm[key])*log((1-p1)/(1-p0));
								}
								position++;
							}
							free(temp);
						}
					}		
				}
			}
		}
		if(!child) {
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
		}
	}
	return 0;
}
