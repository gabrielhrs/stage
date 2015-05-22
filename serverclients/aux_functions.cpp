#include <iostream>
  using std::endl;
#include <boost/random/mersenne_twister.hpp>
  using boost::mt19937;
#include <boost/random/poisson_distribution.hpp>
  using boost::poisson_distribution;
#include <boost/random/variate_generator.hpp>
  using boost::variate_generator;

extern "C" double poisson_interval_generator(double mean){
	mt19937 gen;
	gen.seed(std::clock());
	poisson_distribution<int> p_dist(mean);
	variate_generator< mt19937, poisson_distribution<int> > rvt(gen, p_dist);
	return rvt();
}
