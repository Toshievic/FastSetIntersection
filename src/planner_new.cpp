#include "../include/planner_new.hpp"


std::vector<int> get_plan(Query *q, LabeledGraph *g, std::string &executor_name) {
    // 現状探索順序はGraphFlowが選択した順序をそのまま使っている
    std::unordered_map<std::string, std::vector<int>> graphflow_orders = {
        {"Q1_n", {0,1,2}},
        {"Q1", {0,1,2}},
        {"Q2_n", {0,2,1,3}},
        {"Q2", {0,2,1,3}},
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
        {"Q6", {2,3,1,0}},
        {"Q7_n", {0,1,2,3,4}},
        {"Q7_n_amazon", {3,4,2,1,0}},
        {"Q7", {3,4,2,1,0}},
    };

    return graphflow_orders[q->query_name];
}
