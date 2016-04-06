/*
 * Utility.cpp
 *
 *  Created on: 13 avr. 2015
 *      Author: Gabriel
 */

#include <iostream>
 using std::cout; using std::endl;
#include <fstream>
#include <cmath>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <exception>

namespace utility {

	/* Comparison functions */

	bool greaterThanXY(double x, double y) {
		return x > y;
	}

	bool smallerThanXY(double x, double y) {
		return x < y;
	}

	/* Numerical approximations for n */

	double invBinCummDistApprox67(double q) {

		double a0 = 2.30753;
		double a1 = 0.27061;
		double b1 = 0.99229;
		double b2 = 0.04481;

		double n = sqrt(log(1/(pow(q,2))));

		return (n-((a0+a1*n)/(1+b1*n+b2*pow(n,2))));

	}

	double invBinCummDistApprox68(double q) {

		double a0 = 2.515517;
		double a1 = 0.802853;
		double a2 = 0.010328;
		double b1 = 1.432788;
		double b2 = 0.189269;
		double b3 = 0.001308;

		double n = sqrt(log(1/(pow(q,2))));

		return (n-((a0+a1*n+a2*pow(n,2))/(1+b1*n+b2*pow(n,2)+b3*pow(n,3))));

	}

	double nApprox67(double a, double b, double p0, double p1) {

		return pow((invBinCummDistApprox67(a)*sqrt(p0*(1-p0))+invBinCummDistApprox67(b)*sqrt(p1*(1-p1))),2)/pow((p0-p1),2);
	}

	double nApprox68(double a, double b, double p0, double p1) {

		return pow((invBinCummDistApprox68(a)*sqrt(p0*(1-p0))+invBinCummDistApprox68(b)*sqrt(p1*(1-p1))),2)/pow((p0-p1),2);
	}

	/* F(c,n,p) */

	double fcnp(double c, double n, double p) {

		int i;
		double r = 0.0;

		for(i = 0; i <= c; i++) {
			try {
				r = r + (boost::math::binomial_coefficient<double>(n,i)*pow(p,i)*pow(1.0-p,n-i));
			}
			catch(std::exception& e) {
				cout << "Fcnp: " << e.what() << endl;
			}
		}

		return r;
	}

	/* F^-1(c,n,p) */

	double invFcnp(double c, double n, double p) {

		int i = 0;
		double r = 0.0;

		while(r <= c) {
			try {
				double delta = (boost::math::binomial_coefficient<double>(n,i)*pow(p,i)*pow(1-p,n-i));
				if(r + delta <= c) {
					r = r + delta;
					i++;
				}
				else
					break;
			}
			catch(std::exception& e) {
				cout << "invFcnp: " << e.what() << endl;
				exit(0);
			}
		}

		return (double) i;
	}

	/* Single-Sampling-Plan as implemented by Younes */

	std::pair<double,double> singleSamplingPlan(double p0, double p1, double a, double b) { /* change from double to int ??? */
		double nmin = 1.0;
		double nmax = -1.0;
		double n = nmin;
		
		if(p1 == 0 && p0== 1) {
			return std::pair<double,double>(1.0, 0.0);
		}
		else if(p1 == 0 && p0 < 1) {
			return std::pair<double,double>((log(a)/(log(1-p0))), 0.0);
		}
		else if(p1 > 0 && p0 == 1) {
			return std::pair<double,double>((log(b)/log(p1)),((log(b)/log(p1))-1));
		}

		while(nmax < 0 || nmin < nmax) {

			double x0 = invFcnp(a,n,p0);
			double x1 = invFcnp(1-b,n,p1);

			if(x0 > x1 && x0 >= 0) { /* originally x0 >= x1 */
				nmax = n;
			}
			else {
				nmin = n + 1;
			}
			if(nmax < 0) {
				n = 2 * n;
			}
			else {
				n = floor((nmin + nmax)/2.0);
			}
		}

		n = nmax - 1;

		double c0;
		double c1;

		do {
			n++;
			c0 = floor(invFcnp(a,n,p0));
			c1 = ceil(invFcnp(1-b,n,p1));
		} while(!(c0 >= c1));

		cout << "n: " << n << endl;
		cout << "(c0 + c1)/2: " << floor((c0+c1)/2) << endl;

		if(fcnp(floor((c0+c1)/2),n,p0) > a || 1 - fcnp(floor((c0+c1)/2),n,p1) > b) {
			throw "Failed to establish an optimal sampling plan.";
			exit(0);
		}

		return std::pair<double,double>(n, floor((c0+c1)/2));
	}

	/* SPRT */

	double fm(double p0, double p1, double m, double dm) {

		return (pow(p1,dm)*pow(1-p1,m-dm))/(pow(p0,dm)*pow(1-p0,m-dm)); 
	}

	void plot(int n_int, int n_min, int n_max, double u, double s, std::string n_var) {

		std::ofstream f_plot;
		f_plot.open(std::string(std::string("plot_") + std::string(n_var)).c_str());
		f_plot << "reset" << endl;
		f_plot << "n = " << n_int << endl; // #number of intervals
		f_plot << "max = " << n_max << endl; //. #max value
		f_plot << "min = " << n_min << endl; // #min value
		f_plot << "width = (max-min)/n" << endl; // #interval width
		//function used to map a value to the intervals
		f_plot << "set xrange [min:max]" << endl;
		f_plot << "set yrange [0:]" << endl;
		//to put an empty boundary around the data inside an autoscaled graph.
		f_plot << "set offset graph 0.05,0.05,0.05,0.0" << endl;
		f_plot << "set xtics 1" << endl; //min,(max-min)/5,max" << endl;
		f_plot << "set boxwidth width*0.9" << endl;
		f_plot << "set style fill solid 0.5" << endl; // #fillstyle 
		f_plot << "set tics out nomirror" << endl;
		f_plot << "set xlabel " << "\"" + n_var + "\"" << endl;
		f_plot << "set ylabel " << "\"Relative Frequency\"" << endl;
		//gauss
		f_plot << "s = " << s << endl;
		f_plot << "u = " << u << endl;
		f_plot << "gauss(x) = (1/(sqrt(2*s*pi)))*exp((-(x-u)**2)/(2*s))" << endl;
		//count and plot
		f_plot << "plot \"data-" + n_var + ".dat\" w boxes, gauss(x)" << endl;
		f_plot.close();
	}

}