#ifndef MASTER_HPP
#define MASTER_HPP

#include <unordered_map>
#include <string>

using namespace std;


// コード内でのデータセットの識別子とファイルパスの対応
// hppファイル内でdataset, queryを初期化すると、変更する際に全て再コンパイルする必要があることからcpp内で実体化
extern unordered_map<string,string> dataset_master, query_master;

// name: epinionsやtestなど
// master_type: dataset or query
string find_path(char* name, string master_type);


#endif