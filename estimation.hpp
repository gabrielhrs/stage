#ifndef ESTIMATION_
#define ESTIMATION_

#include <iostream>
using std::cout; using std::endl;

class Estimation {

	bool concluded;
	double confidence_interval;
	double coverage_probability;
	std::string variable;

	public:
		Estimation();
		Estimation(double,double,std::string);
		void set_concluded(bool);
		void set_confidence_interval(double);
		void set_coverage_probability(double);
		void set_variable(std::string);
		bool is_concluded();
		double get_coverage_probability();
		double get_confidence_interval();
		std::string get_variable();
		void print();
};

inline Estimation::Estimation() {
	concluded = 0;
}

inline Estimation::Estimation(double v1, double v2, std::string s) {
	confidence_interval = v1;
	coverage_probability = v2;
	variable = s;
	concluded = 0;
}

inline void Estimation::set_concluded(bool v) {
	concluded = v;
}

inline void Estimation::set_confidence_interval(double v) {
	confidence_interval = v;
}

inline void Estimation::set_coverage_probability(double v) {
	coverage_probability = v;
}

inline void Estimation::set_variable(std::string s) {
	variable = s;	
}

inline bool Estimation::is_concluded() {
	return concluded;
}

inline double Estimation::get_coverage_probability() {
	return coverage_probability;
}

inline double Estimation::get_confidence_interval() {
	return confidence_interval;
}

inline std::string Estimation::get_variable() {
	return variable;
}

inline void Estimation::print() {
	std::cout << "E(" << "\"" << get_variable() << "\"" << "," << get_coverage_probability() << "," << get_confidence_interval() << ")";
}

#endif /* ESTIMATION_ */