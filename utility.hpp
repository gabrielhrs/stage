/*
 * utility.hpp
 *
 *  Created on: 13 avr. 2015
 *      Author: Gabriel
 */

#ifndef UTILITY_HPP_
#define UTILITY_HPP_

namespace utility {

	bool greaterThanXY(double x, double y);

	bool smallerThanXY(double x, double y);

	double invBinCummDistApprox67(double q);

	double invBinCummDistApprox68(double q);

	double nApprox67(double a, double b, double p0, double p1);

	double nApprox68(double a, double b, double p0, double p1);

	double fcnp(double c, double n, double p);

	double invFcnp(double c, double n, double p);

	std::pair<double,double> singleSamplingPlan(double p0, double p1, double a, double b);
	
	double fm(double p0, double p1, double m, double dm);

	void plot(int n_int, int n_min, int n_max, double u, double s, std::string n_var);

}

#endif /* UTILITY_HPP_ */