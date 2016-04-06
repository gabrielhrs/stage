To compile:

g++ main.cpp ./parser/parser.cpp ./utility.cpp ./sst.cpp ./srpt.cpp ./chow_robbins.cpp ./serverclients/aux_functions.so -I ./ -o main

To run an example:

./main ./serverclients/serverclients_mv ./serverclients/platform.xml ./serverclients/deployment.xml ./serverclients/formulae.txt
