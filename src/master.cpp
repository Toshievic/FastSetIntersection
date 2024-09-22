#include <cstdlib>

#include "../include/master.hpp"


// 環境変数からデータセット・クエリのパスを読み取る
string dataset_base_path = getenv("DATASET_BASE_PATH");
string query_base_path = getenv("QUERY_BASE_PATH");

unordered_map<string, string> dataset_master = {
    {"epinions", dataset_base_path+"epinions/"},
    {"amazon", dataset_base_path+"amazon/"},
    {"google", dataset_base_path+"google/"},
    {"berkstan", dataset_base_path+"berkstan/"},
    {"patent", dataset_base_path+"patent/"},
    {"youtube", dataset_base_path+"youtube/"},
    {"test", dataset_base_path+"test/"}
};

unordered_map<string, string> query_master = {
    {"graphflow", query_base_path+"graphflow/"},
    {"graphflow_epinions", query_base_path+"graphflow_epinions/"},
    {"base", query_base_path+"base/"},
    {"base2", query_base_path+"base2/"},
    {"base3", query_base_path+"base3/"},
    {"test", query_base_path+"test/"},
};
