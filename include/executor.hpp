#ifndef EXECUTOR_NEW_HPP
#define EXECUTOR_NEW_HPP

#include "util.hpp"
#include "planner.hpp"

#include <iostream>
#include <memory>
#include <unordered_set>


class Executor {
public:
    std::string executor_name;
    LabeledGraph *g;
    Query *q;
    std::vector<int> order;
    std::unordered_map<std::string, double> time_stats;
    std::unordered_map<std::string, unsigned long long> exec_stats;
    unsigned *assignment;
    unsigned long long result_size;
    unsigned long long intersection_count;

    virtual void init() {}
    virtual void run(std::unordered_map<std::string, std::string> &options) {}
};


class GenericExecutor : public Executor {
private:
    std::vector<std::vector<std::pair<unsigned, int>>> plan;
    std::unordered_map<int,int> available_level;
    std::unordered_map<int,std::vector<std::pair<int,int>>> cache_switch;
    std::unordered_set<int> has_full_cache;
public:
    int scan_vl;
    std::vector<std::vector<int>> match_nums;
    std::vector<std::vector<unsigned *>> result_store;

    GenericExecutor() {}
    GenericExecutor(std::string &data_dirpath, std::string &query_filepath) {
        executor_name = "generic";
        std::cout << "=== Generic Executor ===" << std::endl;
        g = new LabeledGraph(data_dirpath);
        q = new Query(query_filepath);
        order = get_plan(q, g, executor_name);
    }

    void init();
    void run(std::unordered_map<std::string, std::string> &options);
    void join();
    void cache_join();

    void recursive_join(int depth);
    void recursive_cache_join(int depth);
    void find_assignables(int depth);
    void summarize_result(std::unordered_map<std::string, std::string> &options);
};


class AggExecutor : public GenericExecutor {
private:
    int v_first; // scanする頂点の番号
    std::vector<std::vector<std::vector<std::pair<unsigned, int>>>> general_plan;
    std::vector<int> agg_order, general_order; // 経由頂点入替を考慮して集約側が参照するorderとhub側が参照するorderを分けて作る
    std::vector<std::vector<std::pair<unsigned, int>>> agg_plan;
    std::vector<std::vector<int>> agg_detail, general_detail; // 経由頂点を保持

    std::vector<std::vector<unsigned *>> intersect_store;
    std::vector<std::vector<std::vector<unsigned *>>> ptr_store; // 経由頂点用

    std::vector<std::vector<std::pair<int,int>>> cache_reset; // 割り当て更新の際にどのdepthのキャッシュをどのステージまで戻すか
    std::vector<int> start_from; // 次のdepthのintersectionをどのstageから始めるか
    std::vector<int> cache_set; // あるdepthの処理を終了した際にそのdepthのキャッシュをどのステージまで進めるか
public:
    AggExecutor(std::string &data_dirpath, std::string &query_filepath) {
        executor_name = "agg";
        std::cout << "=== Agg Executor ===" << std::endl;
        g = new LabeledAggGraph(data_dirpath);
        q = new Query(query_filepath);
        order = get_plan(q, g, executor_name);
    }

    void init();
    void run(std::unordered_map<std::string, std::string> &options);
    void join();
    void cache_join();

    void recursive_agg_join(int depth, int stage, std::vector<int> &use_agg);
    void recursive_agg_cache_join(int depth, int stage, std::vector<int> &use_agg);
};


class ExecutorCaller {
public:
    static std::unique_ptr<Executor> call(std::string &method, std::string &data_dirpath, std::string &query_filepath) {
        if (method == "generic") {
            return std::make_unique<GenericExecutor>(data_dirpath, query_filepath);
        }
        else if (method == "agg") {
            return std::make_unique<AggExecutor>(data_dirpath, query_filepath);
        }
        else {
            std::cerr << "Error: Unknown method '" << method << "'\n";
            exit(1); 
        }
    }
};

#endif