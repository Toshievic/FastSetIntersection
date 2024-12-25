#include "include/executor_new.hpp"

#include <cstring>


int main(int argc, char* argv[]) {
    // main.cppの引数: ./exec.out --data_dirpath <data_dirpath> --query_filepath <query_filepath> --method <method> --items <item> .. <item>
    // data_dirpath: グラフデータへのパス
    // query_filepath: クエリファイルへのパス
    // method: 処理を行うフレームワーク
    // items: 処理の際に使用する拡張データ (複数指定可)

    if (argc < 7) {
        std::cerr << "ArgumentError: Some arguments were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --data_dirpath <data_dirpath> --query_filepath <query_filepath> --method <method> --options <option1=value1> .. <option=value>" << std::endl;
        return 1;
    }

    std::string data_dirpath;
    std::string query_filepath;
    std::string method;
    std::unordered_map<std::string, std::string> options;

    if (std::strcmp(argv[1], "--data_dirpath") == 0) { data_dirpath = argv[2]; }
    else {
        std::cerr << "ArgumentError: The argument 'data_dirpath' were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --data_dirpath <data_dirpath> --query_filepath <query_filepath> --method <method> --options <option1=value1> .. <option=value>" << std::endl;
        return 1;
    }

    if (std::strcmp(argv[3], "--query_filepath") == 0) { query_filepath = argv[4]; }
    else {
        std::cerr << "ArgumentError: The argument 'query_filepath' were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --data_dirpath <data_dirpath> --query_filepath <query_filepath> --method <method> --options <option1=value1> .. <option=value>" << std::endl;
        return 1;
    }

    if (std::strcmp(argv[5], "--method") == 0) { method = argv[6]; }
    else {
        std::cerr << "ArgumentError: The argument 'method' were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --data_dirpath <data_dirpath> --query_filepath <query_filepath> --method <method> --options <option1=value1> .. <option=value>" << std::endl;
        return 1;
    }

    if (argc > 7) {
        if (std::strcmp(argv[7], "--options") == 0) {
            for (int i=8; i<argc; ++i) {
                std::string option(argv[i]);
                std::vector<std::string> key_value = split(option, '=');
                options[key_value[0]] = key_value[1];
            }
        }
        else {
            std::cerr << "ArgumentError: Invalid argument '" << argv[7] << "'\n" <<
            "Usage: "<< argv[0] <<
            " --data_dirpath <data_dirpath> --query_filepath <query_filepath> --method <method> --options <option1=value1> .. <option=value>" << std::endl;
            return 1;
        }
    }

    std::unique_ptr<SimpleExecutor> e = ExecutorCaller::call(method, data_dirpath, query_filepath);

    for (int i=0; i<1; ++i) {
        e->init();
        e->run(options);
    }

    return 0;
}