#ifndef QUERY_HPP
#define QUERY_HPP

#include "master.hpp"

#include <set>
#include <vector>


class Query {
private:
    bool debug;
    unordered_map<string, int> val_index;
public:
    string query_name;
    unordered_map<int,int> variables;
    vector<vector<int>> triples;

    Query() {}

    void load(string file_path);
    void info();
};

vector<Query> load_queries(string query_sets, bool debug);

#endif
