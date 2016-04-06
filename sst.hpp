#ifndef SST_HPP_
#define SST_HPP_

namespace sst {

	bool sstTest(boost::unordered_map<std::string, Hypothesis> hypotheses, boost::unordered_map<std::string, std::pair<double,double> > map_sp,
				 boost::unordered_map<std::string, double> map_dm, int m);

	int sst(std::string model, std::string platform_file, std::string deployment_file, boost::unordered_map<std::string, Hypothesis> hypotheses);

}

#endif /* SST_HPP_ */