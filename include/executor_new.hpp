#ifndef EXECUTOR_NEW_HPP
#define EXECUTOR_NEW_HPP

#include "util_new.hpp"
#include "planner_new.hpp"

#include <iostream>
#include <memory>


class Executor {
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


class GenericExecutor : public Executor {
public:
    unsigned *intersects; // intersection結果の格納場所
    int x_base_idx, y_base_idx, scan_idx;

    GenericExecutor() {}
    GenericExecutor(std::string &data_dirpath, std::string &query_filepath) {
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


class AggExecutor : public GenericExecutor {
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