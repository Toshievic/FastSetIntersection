#ifndef PLANNER_NEW_HPP
#define PLANNER_NEW_HPP

#include "labeled_graph.hpp"
#include "query.hpp"

#include <vector>
#include <string>
#include <unordered_set>

class GraphFlowPlanner {
private:
    Query *q;
    LabeledGraph *g;
    std::unordered_map<std::string, std::vector<int>> graphflow_orders = {
        {"Q1_n", {0,1,2}},
        {"Q1", {0,1,2}},
        {"Q2_n", {0,2,1,3}},
        {"Q2", {2,3,1,0}},
        {"Q3_n", {0,1,3,2}},
        {"Q3_n_slashdot", {0,2,1,3}},
        {"Q3_n_amazon", {1,3,0,2}},
        {"Q3", {1,3,2,0}},
        {"Q4_n", {1,2,3,0}},
        {"Q4", {2,3,1,0}},
        {"Q5_n", {1,2,0,3}},
        {"Q5_n_google", {1,2,3,0}},
        {"Q5", {1,2,3,0}},
        {"Q6_n", {0,1,2,3}},
        {"Q6_n_amazon", {2,3,1,0}},
        {"Q6_n_wiki", {0,2,1,3}},
        {"Q6", {2,3,1,0}},
        {"Q7_n", {0,1,2,3,4}},
        {"Q7_n_amazon", {3,4,2,1,0}},
        {"Q7_n_wiki", {0,3,1,2,4}},
        {"Q7", {3,4,2,1,0}},
    };
public:
    GraphFlowPlanner(Query *q, LabeledGraph *g) {
        this->q = q;
        this->g = g;
    }
    std::vector<int> get_order() { return graphflow_orders[q->query_name]; }
};


class AggPlanner {
private:
    Query *q;
    LabeledGraph *g;
    std::vector<int> best_order;
    std::unordered_map<int,int> best_via_map;
    int min_cost;
public:
    AggPlanner(Query *q, LabeledGraph *g) {
        this->q = q;
        this->g = g;
    }
    std::vector<int> get_order();
    void recursive_search(std::vector<int> &current_order, std::unordered_map<int,int> &via_map,
    std::unordered_set<int> &non_vias, std::vector<std::unordered_set<int>> &dependencies, std::vector<int> &intersects);
};

std::vector<int> get_plan(Query *q, LabeledGraph *g, std::string &executor_name);

#endif