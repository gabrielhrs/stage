#ifndef SRPT_HPP_
#define SRPT_HPP_

namespace srpt {

	bool srptTest(boost::unordered_map<std::string, Hypothesis> hypotheses, boost::unordered_map<std::string, double> map_fm);

	int srpt(std::string model, std::string platform_file, std::string deployment_file, boost::unordered_map<std::string, Hypothesis> hypotheses);

}

#endif /* SRPT_HPP_ */