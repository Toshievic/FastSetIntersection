#include "../include/query.hpp"
#include "../include/util.hpp"


void Query::load(string file_path) {
    query_name = split(split(file_path, '/').back(), '.')[0];
    ifstream ifs_file(file_path);
    vector<vector<string>> raw = read_table(ifs_file, ' ');
    
    int mode = 0;
    int i = 0;
    for (auto &tokens : raw) {
        if (tokens.size() == 0) {
            mode++;
            continue;
        }
        switch (mode)
        {
            case 0: // val_type
                val_index[tokens[0]] = i;
                variables[i] = stoi(tokens[1]);
                ++i;
                break;
            case 1: // triple
                triples.push_back({val_index[tokens[0]], stoi(tokens[1]), val_index[tokens[2]]});
                break;
            default:
                break;
        }
    }
}


void Query::info() {
    cout << "Variables:" << endl;
    print(variables);
    cout << "Triples:" << endl;
    print(triples);
    cout << endl;
}


vector<Query> load_queries(string query_sets, bool debug) {
    if (debug) { cout << "----------------- Loading Queries ----------------" << endl; }
    string dir_path = query_master.at(query_sets);
    vector<string> query_files = get_file_paths(dir_path);

    vector<Query> queries;
    for (auto &query_file : query_files) {
        Query query;
        query.load(query_file);
        queries.push_back(query);
    }

    if (debug) {
        for (auto &query : queries) {
            query.info();
        }
        cout << "----------------- Finish Loading! ----------------" << endl << endl;
    }

    return queries;
}
