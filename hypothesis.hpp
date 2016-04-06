#ifndef HYPOTHESIS_
#define HYPOTHESIS_

#include <iostream>
using std::cout; using std::endl;

typedef bool (*c_function)(double, double);

class Hypothesis { //change this name and change the file extension to hpp
	double p0;
	double p1;
	double v1;
	double v2;
	double alpha;
	double beta;
	bool decided;
	c_function fH0;
	c_function fH1;
 	std::string variable;

	public:
		Hypothesis();
		Hypothesis(double,double,double,double,double,double,c_function,c_function,std::string);
		void set_p0(double);
		void set_p1(double);
		void set_v1(double);
		void set_v2(double);
		void set_alpha(double);
		void set_beta(double);
		void setDecided(bool);		
		void set_variable(std::string);
		void set_fH0(c_function);
		void set_fH1(c_function);
		double get_p0();
		double get_p1();
		double get_v1();
		double get_v2();
		double get_alpha();
		double get_beta();
		std::string get_variable();
		std::string print_fH0();
		std::string print_fH1();
		bool isDecided();	
		c_function get_fH0();
		c_function get_fH1();
		void print();
};

inline Hypothesis::Hypothesis() {
	decided = 0;
}

inline Hypothesis::Hypothesis(double vp0, double vp1, double vv1, double vv2, double va, double vb, c_function f1, c_function f2 ,std::string s1) {
	p0 = vp0;
	p1 = vp1;
	v1 = vv1;
	v2 = vv2;
	alpha = va;
	beta = vb;
	fH0 = f1;
	fH1 = f2;
	variable = s1;
	decided = 0;
}

inline void Hypothesis::set_p0(double v) {
	p0 = v;
}

inline void Hypothesis::set_p1(double v) {
	p1 = v;
}

inline double Hypothesis::get_p0() {
	return p0;
}

inline double Hypothesis::get_p1() {
	return p1;
}

inline void Hypothesis::set_v1(double v) {
	v1 = v;
}

inline double Hypothesis::get_v1() {
	return v1;
}

inline void Hypothesis::set_v2(double v) {
	v2 = v;
}

inline double Hypothesis::get_v2() {
	return v2;
}

inline void Hypothesis::set_alpha(double v) {
	alpha = v;
}

inline double Hypothesis::get_alpha() {
	return alpha;
}

inline void Hypothesis::set_beta(double v) {
	beta = v;
}

inline double Hypothesis::get_beta() {
	return beta;
}

inline void Hypothesis::set_fH0(c_function f) {
	fH0 = f;
}

inline void Hypothesis::set_fH1(c_function f) {
	fH1 = f;
}

inline c_function Hypothesis::get_fH0() {
	return fH0;
}

inline c_function Hypothesis::get_fH1() {
	return fH1;
}

inline std::string Hypothesis::print_fH0() {
	return get_fH0()(0,1)? "<" : ">"; 	
}

inline std::string Hypothesis::print_fH1() {
	return get_fH1()(0,1)? "<" : ">"; 	
}

inline void Hypothesis::set_variable(std::string s) {
	variable = s;
}

inline std::string Hypothesis::get_variable() {
	return variable;
}

inline void Hypothesis::setDecided(bool v) {
	decided = v;
}

inline bool Hypothesis::isDecided() {
	return decided;
}

inline void Hypothesis::print() {
	std::cout << "HO:" << "p0" << ">" << get_p0() << "[" << "\"" << get_variable() << "\"" << print_fH0() << get_v1() << "],"
				<< "H1:" << "p1" << "<" << get_p1() << "[" << "\"" << get_variable() << "\"" << print_fH1() << get_v2() << "]";
}

#endif /* HYPOTHESIS_ */