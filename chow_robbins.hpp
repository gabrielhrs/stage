#ifndef CHOW_ROBBINS_HPP_
#define CHOW_ROBBINS_HPP_

namespace chowrobbins {

	bool cAndRTest(boost::unordered_map<std::string, Estimation> formulae, boost::unordered_map<std::string, double> map_d_est);

	int evaluate(std::string model, std::string platform_file, std::string deployment_file, boost::unordered_map<std::string, Estimation> formulae);

}

#endif /* CHOW_ROBBINS_HPP_ */