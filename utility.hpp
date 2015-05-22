/*
 * utility.hpp
 *
 *  Created on: 13 avr. 2015
 *      Author: Gabriel
 */

#ifndef UTILITY_HPP_
#define UTILITY_HPP_

namespace utility {

double invBinCummDistApprox67(double q);

double invBinCummDistApprox68(double q);

double nApprox67(double a, double b, double p0, double p1);

double nApprox68(double a, double b, double p0, double p1);

double fcnp(double c, double n, double p);

double invFcnp(double c, double n, double p);

std::pair<double, double> singleSamplingPlan(double p0, double p1, double a, double b);

double fm(double p0, double p1, double m, double dm);

}

#endif /* UTILITY_HPP_ */
