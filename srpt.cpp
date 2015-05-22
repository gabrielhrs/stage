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

using namespace std;
#define BUFSIZE 1024
#define BATCHSIZE 8

int main(int argc, char *argv[]) {

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */
	
	double v_test = atof(argv[4]);

	double p0 = 0.9;
	double p1 = 0.7;
	double alpha = 0.2;
	double beta = 0.1;

	if(p0 == 1 || p1 == 0) {
		std::string const command = "./sst" + std::string(model) + " " + std::string(platform_file)  
										 + " " + std::string(deployment_file) 
										 + " " + boost::lexical_cast<string>(v_test);
		system(command.c_str());
	}
	else {

		std::cout << "-- Running SPRT --" << endl;

		int m = 0;
		double fm = 0;

		pid_t pid[BATCHSIZE]; 
		int fd[BATCHSIZE][2]; 

		int child = 0; 
		int child_idx = -1; 
		int g_index = 0;
		int g_counter; 

		int dm = 0;

		while(log(beta/(1-alpha)) < fm && fm < log((1-beta)/alpha)) {
			if(!child) {
				for(g_counter = 0; (g_counter < BATCHSIZE && log(beta/(1-alpha)) < fm && fm < log((1-beta)/alpha)); g_counter++) { /* generates a group of children processes */	
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
					double v = atof(buffer);

					if(v > v_test) {
						dm++;
					}	
					close(fd[i%BATCHSIZE][0]);
					fm = dm*log(p1/p0)+(m-dm)*log((1-p1)/(1-p0));
				}
			}
		}
		if(!child) {
			if(fm <= log(beta/(1-alpha))){
				std::cout << "Runs: " << m << endl;
				std::cout << "Fail to reject H0." << endl;
			}
			else {
				std::cout << "Runs: " << m << endl;
				std::cout << "H0 is rejected. H1 is accepted." << endl;
			}
		}
	}
	return 0;
}
