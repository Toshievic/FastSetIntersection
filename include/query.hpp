#ifndef QUERY_NEW_HPP
#define QUERY_NEW_HPP

#include <string>
#include <unordered_map>
#include <vector>


class Query {
public:
    std::string query_name;
    std::unordered_map<int, int> vars;
    std::vector<std::vector<int>> triples;

    Query() {}
    Query(std::string &query_filepath);
};

#endif