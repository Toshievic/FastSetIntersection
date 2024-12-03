#include "../include/query_new.hpp"
#include "../include/util_new.hpp"

#include <fstream>
#include <sstream>
#include <iostream>


Query::Query(std::string &query_filepath) {
    query_name = split(split(query_filepath, '/').back(), '.')[0];
    std::ifstream fin(query_filepath);
    if (!fin) {
        std::cerr << "Cannot open file: " << query_filepath << std::endl;
        exit(1);
    }

    std::string line_buf;
    std::vector<std::string> word_buf;
    std::vector<std::vector<std::string>> query_raw;
    while (getline(fin, line_buf)) {
        std::istringstream i_stream(line_buf);
        word_buf = split(line_buf, ' ');
        query_raw.push_back(word_buf);
    }

    int section = 0;
    int i = 0;
    std::unordered_map<std::string, int> var_idx;
    for (auto &tokens : query_raw) {
        if (tokens.size() == 0) {
            ++section;
            continue;
        }
        switch (section)
        {
            case 0: // val_type
                var_idx[tokens[0]] = i;
                vars[i] = stoi(tokens[1]);
                ++i;
                break;
            case 1: // triple
                triples.push_back({var_idx[tokens[0]], stoi(tokens[1]), var_idx[tokens[2]]});
                break;
            default:
                break;
        }
    }
}
