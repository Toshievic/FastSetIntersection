#include <cstdlib>
#include <iostream>

#include "../include/master.hpp"


unordered_map<string, string> dataset_master = {
    {"karate", "karate/"},
    {"epinions", "epinions/"},
    {"ego_twitter", "ego_twitter/"},
    {"slashdot", "slashdot/"},
    {"wiki_topcats", "wiki-topcats/"},
    {"amazon", "amazon/"},
    {"google", "google/"},
    {"berkstan", "berkstan/"},
    {"patent", "patent/"},
    {"live_journal", "live_journal/"},
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
    else if (master_type == "query") {
         // 環境変数からクエリのパスを読み取る
        string workdir_path = getenv("WORKDIR_PATH");
        return workdir_path + "query/" + query_master[name];
    }
    else if (master_type == "input") {
         // 環境変数から入力データのパスを読み取る
        string workdir_path = getenv("WORKDIR_PATH");
        return workdir_path + "data/";
    }

    return "";
}
