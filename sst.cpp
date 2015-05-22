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

int main(int argc, char *argv[]) {

	std::cout << "-- Running SST --" << endl;

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */

/*
	double p0 = atof(argv[4]);
	double p1 = atof(argv[5]);
	double alpha = atof(argv[6]);
	double beta = atof(argv[7]);
*/
	double v_test = atof(argv[4]);
/*
	double v_test = 14.0;
*/
	double p0 = 0.9;
	double p1 = 0.7;
	double alpha = 0.2;
	double beta = 0.1;

	pair<double,double> sampling_plan = utility::singleSamplingPlan(p0,p1,alpha,beta);

	int n = sampling_plan.first;
	int c = sampling_plan.second;

	int m = 0;
	int dm = 0;

	pid_t pid[BATCHSIZE]; 
	int fd[BATCHSIZE][2];
	int child = 0; 
	int child_idx = -1; 
	int g_index = 0;
	int g_counter;

	while(dm <= c && (dm+n-m) > c) {
		if(!child) {
			for(g_counter = 0; (g_counter < BATCHSIZE && dm <= c && dm+n-m > c); g_counter++) { /* generates a group of children processes */	
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
			}
		}
	}
	if(!child) {
		if(dm > c) {
			std::cout << "Runs: " << m << endl;
			std::cout << "Fail to reject H0." << endl;
		}
		else {
			std::cout << "Runs: " << m << endl;
			std::cout << "H0 is rejected. H1 is accepted." << endl;
		}
	}	

	return 0;
}
