#include <cstdlib>
#include <iostream>

#include "../include/master.hpp"


unordered_map<string, string> dataset_master = {
    {"epinions", "epinions/"},
    {"amazon", "amazon/"},
    {"google", "google/"},
    {"berkstan", "berkstan/"},
    {"patent", "patent/"},
    {"youtube", "youtube/"},
    {"test", "test/"}
};

unordered_map<string, string> query_master = {
    {"graphflow", "graphflow/"},
    {"graphflow_epinions", "graphflow_epinions/"},
    {"base", "base/"},
    {"base2", "base2/"},
    {"base3", "base3/"},
    {"test", "test/"},
};


string find_path(string name, string master_type) {
    if (master_type == "dataset") {
         // 環境変数からデータセットのパスを読み取る
        string dataset_base_path = getenv("DATASET_BASE_PATH");
        return dataset_base_path + dataset_master[name];
    }
    
    if (master_type == "query") {
         // 環境変数からクエリのパスを読み取る
        string workdir_path = getenv("WORKDIR_PATH");
        return workdir_path + "query/" + query_master[name];
    }

    return "";
}
