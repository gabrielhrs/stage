#include <iostream>
using std::cout; using std::endl;
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

	char * model = argv[1]; /* model to be executed */
	char * platform_file = argv[2]; /* plataform file */
	char * deployment_file = argv[3]; /* deployment file */
	int runs = atoi(argv[4]); /* total number of process to launch */

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

	/* Gaussian Confidence Interval */

	double z = boost::math::erf_inv(1.0 - 0.05/2.0);

	/* Chows and Robbins */

	double conf_level = 0.95;
	double error = 0.05;
	double a = boost::math::erf_inv(1.0 - error/2.0);
	double d = 0.1;

	using namespace boost::accumulators;

	accumulator_set<double, features<tag::min, tag::max, tag::mean, tag::variance, tag::median, tag::sum> > acc;

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

			for (i = g_index * BATCHSIZE; i < k; i++){
	 			char buffer[BUFSIZE] = "";
				waitpid(pid[i%BATCHSIZE], NULL, 0);
				close(fd[i%BATCHSIZE][1]);
				read(fd[i%BATCHSIZE][0], &buffer, sizeof(buffer));
				double v = atof(buffer);
				data[i] = v;
				acc(v);
				acc_b(v);

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
		cout << "-- Gaussian Interval --" << endl;

		cout << "Avg in [";
		cout << (mean(acc) - z*sqrt(variance(acc))/sqrt(k)); 
		cout << "," << (mean(acc) + z*sqrt(variance(acc))/sqrt(k)) << "]" << endl; 
		//cout << "] = 0.95" << endl;    

		double delta = 2*z*sqrt(variance(acc))/sqrt(k);

		cout << "delta: " << delta << endl;
		*/

		cout << "-- Sequential Samling Plan --" << endl;
		double n_fm = utility::fm(p0,p1,k,n_above);

		if(n_fm >= n_A && k >= sampling_plan.first && n_above >= sampling_plan.second) {
			cout << "Condition A is verified" << endl;
			cout << "n: " << sampling_plan.first << endl;
			cout << "c: " << sampling_plan.second << endl;
			cout << "alpha: " << alpha << endl;
			cout << "beta: " << beta << endl;
			cout << "fm: " << n_fm << endl;
		}
		else if(n_fm <= n_B && k >= sampling_plan.first && n_above >= sampling_plan.second) {
			cout << "Condition B is verified" << endl;
			cout << "n: " << sampling_plan.first << endl;
			cout << "c: " << sampling_plan.second << endl;
			cout << "alpha: " << alpha << endl;
			cout << "beta: " << beta << endl;
			cout << "fm: " << n_fm << endl;
		}
		else {
			cout << "None of the conditions are verified thus far" << endl;
		}


		cout << "-- Central Limit -- " << endl;

		if(utility::chowAndRobbinsTest(k, sum(acc), mean(acc), d, a)){
			cout << "-- Chows and Robbins --" << endl;
			cout << "Confidence level: " << conf_level << endl;
			cout << "Confidence interval (2d): " << 2 * d << endl;
			cout << "Avg in : [" << mean(acc) - d << "," << mean(acc) + d << "]" << endl;
		}

		/*
			
		cout << "At " << k << " fm: " << n_fm << endl;
			
		cout << "N above: " << n_above << endl;
		cout << "N below: " << n_below << endl;
			
		if(n_fm >= n_A) {
			cout << "Condition A is verified" << endl;
		}
		else if(n_fm <= n_B) {
			cout << "Condition B is verified" << endl;
		}
		else {
			cout << "None of the conditions are verified thus far" << endl;
		}
			
		cout << n_fm << endl;

		std::cout << g_index << " groups" << endl;
		*/
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
	}
	return 0;
}

