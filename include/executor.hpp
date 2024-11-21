#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "../include/labeled_graph.hpp"
#include "../include/query.hpp"
#include "../include/util.hpp"

#include <unordered_set>


class GenericJoin {
public:
    bool debug;
    string executor_name;
    unordered_map<string,int> method_dict = {
        {"multiway", 0},
        {"pairwise", 1},
        {"bitset", 2},
        {"2hop", 3}
    };
    int method_id;
    double runtime;
    vector<unsigned*> *stats;
    LabeledGraph *lg;
    Query *q;
    int num_variables;
    vector<int> vertex_order;
    unordered_map<int,int> v_index;

    int scanned_el, scanned_sl, scanned_dl;
    bool scanned_dir;
    vector<vector<tuple<int,int,int,int>>> descriptors;

    pair<unsigned*, unsigned*> *indices;

    vector<unsigned> scanned_srcs;
    vector<unsigned> scanned_dsts;
    unsigned *tmp_result;
    unsigned *keys;
    vector<unsigned> *intersection_result;
    unsigned *match_nums;
    unsigned long result_size;
    unsigned intersection_count;
    unsigned long match_num_total, al_len_total;
    unsigned num_comparison;
    unordered_map<int, unsigned> update_num, empty_num;
    unordered_map<int, double> lev_runtime, empty_runtime;

    vector<vector<unsigned long long>> validate_pool;

    // keyがキャッシュのキーのうちQVOが最も遅い頂点, valueがキャッシュ値
    unordered_map<int, vector<int>> cache_switch;
    unordered_map<int, bool> cache_available;
    bitset<BITSET_SIZE> filter_bs;
    vector<unsigned> bit_num_stat;

    GenericJoin() {}
    GenericJoin(bool b, vector<unsigned*> *stats) {
        debug = b;
        this->stats = stats;
        this->executor_name = "GJ";
    }

    void decide_plan(LabeledGraph *lg, Query *query);
    virtual void setup();

    void run(string &method_name);
    void get_single_assignment();
    virtual void multiway_join();
    virtual void recursive_join(int current_depth);
    virtual void find_assignables(int current_depth); // 一気に全部intersection
    virtual void find_assignables_v2(int current_depth); // binary intersection
    virtual void find_assignables_with_bitset(int current_depth); // binary intersection
    virtual void find_assignables_with_2hop(int current_depth);

    void summarize();
};


class AlphaGenericJoin : public GenericJoin {
private:
    vector<vector<unsigned *>> result_store;
    vector<vector<int>> match_nums;
public:
    unordered_map<int,int> available_level;
    unordered_map<int,vector<pair<int,int>>> cache_switch;
    unordered_set<int> has_full_cache;
    vector<bitset<BITSET_SIZE>*> bs_store;
    unsigned *idxes;
    vector<vector<unsigned long long>> validate_pool;
    int *calc_level; // BitFilterによって空集合判定された際にキャッシュが利用可能なレベル
    unsigned dist_counter[3];

    using GenericJoin::GenericJoin;
    AlphaGenericJoin(bool b, vector<unsigned*> *stats) {
        debug = b;
        this->stats = stats;
        this->executor_name = "AGJ";
    }

    void setup();
    void multiway_join();
    void recursive_join(int current_depth);
    void find_assignables(int current_depth);
    void find_assignables_v2(int current_depth);
    void find_assignables_with_bitset(int current_depth);
    void find_assignables_with_2hop(int current_depth);
    void find_assignables_exp(int current_depth);
};


class BetaGenericJoin : public AlphaGenericJoin {
public:
    vector<vector<unsigned **>> result_store;
    vector<vector<int>> dist_base;
    vector<vector<int *>> match_nums;
    unsigned int** tmp_result_store;

    using AlphaGenericJoin::AlphaGenericJoin;
    BetaGenericJoin(bool b, vector<unsigned*> *stats) {
        debug = b;
        this->stats = stats;
        this->executor_name = "BGJ";
    }

    void setup();
    void recursive_join(int current_depth);
    void find_assignables(int current_depth);
    void find_assignables_v2(int current_depth);
    void find_assignables_with_2hop(int current_depth);
};


class GammaGenericJoin : public AlphaGenericJoin {
public:
    vector<vector<unsigned *>> result_store;
    vector<vector<int>> dist_base;
    vector<vector<unsigned *>> match_nums;
    pair<unsigned*,unsigned*> indices[6];
    unsigned deg;

    using AlphaGenericJoin::AlphaGenericJoin;
    GammaGenericJoin(bool b, vector<unsigned*> *stats) {
        debug = b;
        this->stats = stats;
        this->executor_name = "GGJ";
    }

    void setup();
    void recursive_join(int current_depth);
    void find_assignables(int current_depth);
};

#endif