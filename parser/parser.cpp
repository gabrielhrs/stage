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
#include <complex>
#include <fstream>

#include "estimation.hpp"
#include "hypothesis.hpp"
#include "utility.hpp"
#include "chow_robbins.hpp"
#include "srpt.hpp"
#include "sst.hpp"

namespace parser {

    using boost::spirit::qi::double_;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::lit;
    using boost::spirit::qi::lexeme;
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::ascii::space;
    using boost::spirit::ascii::string;
    using boost::phoenix::ref;

    template <typename Iterator>
    bool parse_estimation_formula(Iterator first, Iterator last, std::vector<Estimation>& v)
    { 
        double v1;
        double v2;
        std::string s;

        boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> quoted_string;
        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

        bool r = phrase_parse(first, last,

            "+E(" >> quoted_string[ref(s) = _1] >> ',' >> double_[ref(v1) = _1] >> ',' >> double_[ref(v2) = _1] >> ')',

            space);

        if (!r || first != last) // fail if we did not get a full match
            return false;
        Estimation e(v1,v2,s);
        v.push_back(e);
        return r;
    }

    template <typename Iterator>
    bool parse_hypothesis_formula_g(Iterator first, Iterator last, std::vector<Hypothesis>& v)
    {
        double p0;
        double p1;
        double v1;
        double v2;
        double alpha;
        double beta;        
        bool (*f1)(double,double);
        bool (*f2)(double,double);
        std::string s;

        boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> quoted_string;
        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

        bool r = phrase_parse(first, last,
            "+(" >> double_[ref(alpha) = _1] >> ',' >> double_[ref(beta) = _1] >> ')' >> 
            "H0:p0>" >> double_[ref(p0) = _1] >> '[' >> quoted_string[ref(s) = _1] >> ">" >> double_[ref(v1) = _1] >> ']' >> "," >>
            "H1:p1<" >> double_[ref(p1) = _1] >> '[' >> quoted_string[ref(s) = _1] >> "<" >> double_[ref(v2) = _1] >> ']',

            space);

        if (!r || first != last) // fail if we did not get a full match
            return false;

        f1 = &utility::greaterThanXY;
        f2 = &utility::smallerThanXY;

        Hypothesis h(p0,p1,v1,v2,alpha,beta,f1,f2,s);
        v.push_back(h);
        return r;
    }

    template <typename Iterator>
    bool parse_hypothesis_formula_l(Iterator first, Iterator last, std::vector<Hypothesis>& v)
    {
        double p0;
        double p1;
        double v1;
        double v2;
        double alpha;
        double beta;
        bool (*f1)(double,double);
        bool (*f2)(double,double);
        std::string s;

        boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> quoted_string;
        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

        bool r = phrase_parse(first, last,

            "+(" >> double_[ref(alpha) = _1] >> ',' >> double_[ref(beta) = _1] >> ')' >> 
            "H0:p0>" >> double_[ref(p0) = _1] >> '[' >> quoted_string[ref(s) = _1] >> "<" >> double_[ref(v1) = _1] >> ']' >> ',' >>
            "H1:p1<" >> double_[ref(p1) = _1] >> '[' >> quoted_string[ref(s) = _1] >> ">" >> double_[ref(v2) = _1] >> ']',

            space);

        if (!r || first != last) // fail if we did not get a full match
            return false;

        f1 = &utility::smallerThanXY;
        f2 = &utility::greaterThanXY;

        Hypothesis h(p0,p1,v1,v2,alpha,beta,f1,f2,s);

        v.push_back(h);
        return r;
    }

    template <typename Iterator>
    bool parse_hypothesis_formula_t(Iterator first, Iterator last, std::vector<Hypothesis>& v)
    {

        /* Correct parsing for most formulas */

        double p0;
        double p1;
        double v1;
        double v2;
        double alpha;
        double beta;
        bool (*f1)(double,double);
        bool (*f2)(double,double);
        std::string s;

        boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> quoted_string;
        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

        bool r = phrase_parse(first, last,
            (
            "+(" >> double_[ref(alpha) = _1] >> ',' >> double_[ref(beta) = _1] >> ')' >> 
            "H0:p0=" >> double_[ref(p0) = _1] >> '[' >> quoted_string[ref(s) = _1] >> "<" >> double_[ref(v1) = _1] >> ']' >> ',' >>
            "H1:p1=" >> double_[ref(p1) = _1] >> '[' >> quoted_string[ref(s) = _1] >> ">" >> double_[ref(v2) = _1] >> ']'
            |
            "+(" >> double_[ref(alpha) = _1] >> ',' >> double_[ref(beta) = _1] >> ')' >> 
            "H0:p0<" >> double_[ref(p0) = _1] >> '[' >> quoted_string[ref(s) = _1] >> "<" >> double_[ref(v1) = _1] >> ']' >> ',' >>
            "H1:p1=" >> double_[ref(p1) = _1] >> '[' >> quoted_string[ref(s) = _1] >> ">" >> double_[ref(v2) = _1] >> ']'
            |
            "+(" >> double_[ref(alpha) = _1] >> ',' >> double_[ref(beta) = _1] >> ')' >> 
            "H0:p0=" >> double_[ref(p0) = _1] >> '[' >> quoted_string[ref(s) = _1] >> "<" >> double_[ref(v1) = _1] >> ']' >> ',' >>
            "H1:p1>" >> double_[ref(p1) = _1] >> '[' >> quoted_string[ref(s) = _1] >> ">" >> double_[ref(v2) = _1] >> ']'
            ),
            space);

        if (!r || first != last) // fail if we did not get a full match
            return false;

        f1 = &utility::smallerThanXY;
        f2 = &utility::greaterThanXY;

        Hypothesis h(p0,p1,v1,v2,alpha,beta,f1,f2,s);

        v.push_back(h);
        return r;
    }

    int parse(std::string model, std::string plataform_file, std::string deployment_file, char * formulae_file) { 

        std::vector<Estimation> v_e;
        std::vector<Hypothesis> v_h;

        pid_t master_pid = getpid();

        std::string line;
        std::ifstream myfile(formulae_file);
        
        if(myfile.is_open()) {
            while(getline(myfile,line)) {   
                parser::parse_estimation_formula(line.begin(), line.end(), v_e);
                parser::parse_hypothesis_formula_g(line.begin(), line.end(), v_h);
                parser::parse_hypothesis_formula_l(line.begin(), line.end(), v_h);
                parser::parse_hypothesis_formula_t(line.begin(), line.end(), v_h);
            }
        }

        boost::unordered_map<std::string, Estimation> estimation_formulae;
        boost::unordered_map<std::string, Hypothesis> hypotheses_formulae_srpt;
        boost::unordered_map<std::string, Hypothesis> hypotheses_formuale_sst;
        int c = 1;

        std::cout << "-- Formula(e) --" << std::endl;

        BOOST_FOREACH(Estimation i, v_e) {
            std::cout << c << ": ";
            i.print();
            std::cout << std::endl;
            estimation_formulae[boost::lexical_cast<std::string>(c)] = i;
            c++;
        }

        BOOST_FOREACH(Hypothesis i, v_h) {
            std::cout << c << ": ";
            i.print();
            std::cout << std::endl;
            if(i.get_p0() == 1 || i.get_p1() == 0) {
                hypotheses_formuale_sst[boost::lexical_cast<std::string>(c)] = i;
            }
            else { 
                hypotheses_formulae_srpt[boost::lexical_cast<std::string>(c)] = i;
            }
            c++;
        }


        if(!estimation_formulae.empty())
            chowrobbins::evaluate(model, plataform_file, deployment_file, estimation_formulae);

        if(master_pid != getpid())
            return 0;

        if(!hypotheses_formulae_srpt.empty())
            srpt::srpt(model, plataform_file, deployment_file, hypotheses_formulae_srpt);

        if(master_pid != getpid())
            return 0;

        if(!hypotheses_formuale_sst.empty())
            sst::sst(model, plataform_file, deployment_file, hypotheses_formuale_sst);

        if(master_pid != getpid())
            return 0;

        myfile.close();
        return 0;
    }
}
