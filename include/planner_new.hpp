#ifndef PLANNER_NEW_HPP
#define PLANNER_NEW_HPP

#include "labeled_graph_new.hpp"
#include "query_new.hpp"

#include <vector>
#include <string>


std::vector<int> get_plan(Query *q, LabeledGraph *g, std::string &executor_name);

#endif