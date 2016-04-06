#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>
#include <fstream>

#include "parser/parser.hpp"
#include "estimation.hpp"
#include "hypothesis.hpp"
#include "utility.hpp"
#include "chow_robbins.hpp"
#include "srpt.hpp"
#include "sst.hpp"

int main (int argc, char *argv[]) {
	
	std::cout << "-- Statistical Model Checker for SimGrid --" << std::endl;

	if (argc < 3) {
    	printf("Usage: %s model platform_file deployment_file formulae_file\n", argv[0]);
    	printf("example: %s serverclients_mv platform.xml deployment.xml formulae\n", argv[0]);
    	exit(1);
  	}

    std::string model = std::string(argv[1]);
    std::string plataform_file = std::string(argv[2]);
    std::string deployment_file = std::string(argv[3]);
    char * formulae_file = argv[4];

    parser::parse(model, plataform_file, deployment_file, formulae_file);

    return 0;
}