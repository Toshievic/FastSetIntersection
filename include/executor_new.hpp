#ifndef EXECUTOR_NEW_HPP
#define EXECUTOR_NEW_HPP

#include "util_new.hpp"
#include "planner_new.hpp"

#include <iostream>
#include <memory>
#include <unordered_set>


class SimpleExecutor {
public:
    std::string executor_name;
    LabeledGraph *g;
    Query *q;
    std::vector<int> order;
    std::unordered_map<std::string, double> time_stats;
    std::unordered_map<std::string, unsigned> exec_stats;
    std::tuple<unsigned,unsigned,unsigned> assignment;

    virtual void init() {}
    virtual void run(std::unordered_map<std::string, std::string> &options) {}
};


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


class SimpleGenericExecutor : public SimpleExecutor {
public:
    unsigned *intersects; // intersection結果の格納場所
    int x_base_idx, y_base_idx, scan_idx;

    SimpleGenericExecutor() {}
    SimpleGenericExecutor(std::string &data_dirpath, std::string &query_filepath) {
        executor_name = "generic";
        std::cout << "=== Generic Executor ===" << std::endl;
        g = new LabeledGraph(data_dirpath);
        q = new Query(query_filepath);
        order = get_plan(q, g, executor_name);
    }

    void init();
    std::unordered_map<std::string, int> collect_meta();
    void run(std::unordered_map<std::string, std::string> &options);
    void join();
    void factorize_join();
    void join_with_lazy_update();

    int intersect(unsigned x, unsigned y); // intersectionの要素数を返す
    int intersect_for_factorize(unsigned *x_first, unsigned *x_last, unsigned y); // intersectionの要素数を返す
    int intersect_lazy_update(unsigned x, unsigned y);
    void summarize_result();
};


class GenericExecutor : public Executor {
public:
    int scan_vl;
    std::vector<std::vector<std::pair<unsigned, int>>> plan;
    std::vector<std::vector<unsigned *>> result_store;
    std::vector<std::vector<int>> match_nums;

    std::unordered_map<int,int> available_level;
    std::unordered_map<int,std::vector<std::pair<int,int>>> cache_switch;
    std::unordered_set<int> has_full_cache;

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


class AggExecutor : public SimpleGenericExecutor {
public:
    int base_idx;

    AggExecutor(std::string &data_dirpath, std::string &query_filepath) {
        executor_name = "agg";
        std::cout << "=== Aggregate Executor ===" << std::endl;
        g = new LabeledAggGraph(data_dirpath);
        q = new Query(query_filepath);
        order = get_plan(q, g, executor_name);
    }

    void init();
    void run(std::unordered_map<std::string, std::string> &options);
    void join();
    void hybrid_join();
    unsigned intersect_agg(unsigned x);
};


class ExecutorCaller {
public:
    static std::unique_ptr<Executor> call(std::string &method, std::string &data_dirpath, std::string &query_filepath) {
        if (method == "generic") {
            return std::make_unique<GenericExecutor>(data_dirpath, query_filepath);
        }
        // else if (method == "agg") {
        //     return std::make_unique<AggExecutor>(data_dirpath, query_filepath);
        // }
        else {
            std::cerr << "Error: Unknown method '" << method << "'\n";
            exit(1); 
        }
    }
};

#endif