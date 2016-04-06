#include <iostream>
  using std::endl;
#include <fstream>
#include <boost/random/mersenne_twister.hpp>
  using boost::mt19937;
#include <boost/random/poisson_distribution.hpp>
  using boost::poisson_distribution;
#include <boost/random/variate_generator.hpp>
  using boost::variate_generator;
#include <boost/graph/directed_graph.hpp> 
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/unordered_map.hpp>

using namespace boost;
typedef adjacency_list< listS, vecS, directedS > Digraph;
  
extern "C" double poisson_interval_generator(double mean);
extern "C" int check_connected_components(int ids[], int best_succ[]);   
extern "C" void print_net(int ids[], int best_succ[]);
extern "C" int build_graph(int ids[], int dests[][24]);
  
/** Generators **/  
  
double poisson_interval_generator(double mean) {
	mt19937 gen;
	gen.seed(std::clock());
	poisson_distribution<int> p_dist(mean);
	variate_generator< mt19937, poisson_distribution<int> > rvt(gen, p_dist);
	return rvt();
}

/** Chord related **/

int check_connected_components(int ids[], int best_succ[]) {
	boost::unordered_map<int, int> index;
	int i = 0;
	while(true) {
		if(ids[i] == -1)
			break;
		index[ids[i]] = i;
		i++;
	}
	int n = i;
	Digraph g(n);
	for(i = 0; i < n; i++) {
		add_edge(i, index.at(best_succ[i]), g);
	}
	std::vector<int> component(num_vertices(g)), discover_time(num_vertices(g));
	return boost::strong_components(g, make_iterator_property_map(component.begin(), get(vertex_index, g)));
}

int build_graph(int ids[], int dests[][24]) {
	int i = 0;
	int j;
	//std::ofstream f_net;
	boost::unordered_map<int, int> index;
	boost::unordered_set<int> filter;
	while(true) {
		if(ids[i] == -1)
			break;
		index[ids[i]] = i;
		i++;
	}
	Digraph g(i);
	int n = i;
	for(i = 0; i < n; i++) {
		for(j = 0; j < 24; j++) {
			if(filter.count(dests[i][j]) == 0) {
				if(i != index.at(dests[i][j]))
					add_edge(i, index.at(dests[i][j]), g);
				filter.insert(dests[i][j]);
			}
		}
		filter.clear();
	}
	//f_net.open("netc.dot");
	//boost::write_graphviz(f_net, g);
	//f_net.close();
	//std::string const command = std::string("dotty ") + std::string("netc.dot");
	//std::system(command.c_str());
	std::vector<int> component(num_vertices(g)), discover_time(num_vertices(g));
	//std::cout << "Number of strong connected components - complete "
	//		  << boost::strong_components(g, make_iterator_property_map(component.begin(), get(vertex_index, g)))
	//		  << std::endl;
	return boost::strong_components(g, make_iterator_property_map(component.begin(), get(vertex_index, g)));
}

void print_net(int ids[], int best_succ[]){
	std::ofstream f_net;
	boost::unordered_map<int, int> index;
	int i = 0;
	while(true) {
		if(ids[i] == -1)
			break;
		index[ids[i]] = i;
		i++;
	}
	int n = i;
	Digraph g(n);
	for(i = 0; i < n; i++) {
		add_edge(i, index.at(best_succ[i]), g);
	}
	f_net.open("net.dot");
	boost::write_graphviz(f_net, g);
	f_net.close();
	std::string const command = std::string("dotty ") + std::string("net.dot");
	std::system(command.c_str());
}