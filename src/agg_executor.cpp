#include "../include/executor_new.hpp"


void AggExecutor::init() {
    std::unordered_map<std::string, int> meta = collect_meta();
    // 各種変数の初期化
    intersects = new unsigned[g->graph_info["max_degree"]];
    base_idx = g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * (2 * (g->graph_info["num_v_labels"] * (
        g->graph_info["num_e_labels"] * meta["dir0"] + meta["el0"]) + meta["dl0"]) + meta["dir1"]) + meta["el1"]) + meta["dl1"]);
    x_base_idx = g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (
        meta["dir0"] * g->graph_info["num_e_labels"] + meta["el0"]) + meta["dl0"]);
    y_base_idx = g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (
        meta["dir1"] * g->graph_info["num_e_labels"] + meta["el1"]) + meta["dl1"]);
    scan_idx = g->graph_info["num_v_labels"] * (g->graph_info["num_v_labels"] * (
        meta["scan_dir"] * g->graph_info["num_e_labels"] + meta["scan_el"]) + meta["scan_sl"]) + meta["scan_dl"];
}


void AggExecutor::run(std::unordered_map<std::string, std::string> &options) {
    Chrono_t start = get_time();
    if (options.contains("hub") && options["hub"] == "drop") {
        hybrid_join();
    }
    else {
        join();
    }
    Chrono_t end = get_time();
    time_stats["join_runtime"] = get_runtime(&start, &end);
    summarize_result();
}


void AggExecutor::join() {
    unsigned first = g->scan_keys[scan_idx];
    unsigned last = g->scan_keys[scan_idx+1];

    unsigned result_size = 0;
    unsigned intersection_count = 0;

    for (int i=first; i<last; ++i) {
        ++intersection_count;
        unsigned num_results = intersect_agg(g->scan_srcs[i]);
        result_size += num_results;
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void AggExecutor::hybrid_join() {
    unsigned first = g->scan_keys[scan_idx];
    unsigned last = g->scan_keys[scan_idx+1];

    unsigned result_size = 0;
    unsigned intersection_count = 0;

    for (int i=first; i<last; ++i) {
        unsigned dst_first = g->scan_crs[i];
        unsigned dst_last = g->scan_crs[i+1];
        for (int j=dst_first; j<dst_last; ++j) {
            ++intersection_count;
            int num_results = intersect(g->scan_srcs[i], g->scan_dst_crs[j]);
            for (int k=0; k<num_results; ++k) {
                assignment = {g->scan_srcs[i], g->scan_dst_crs[j], intersects[k]};
                ++result_size;
            }
        }
        ++intersection_count;
        unsigned num_results = intersect_agg(g->scan_srcs[i]);
        result_size += num_results;
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


unsigned AggExecutor::intersect_agg(unsigned x) {
    unsigned x_idx = x_base_idx + x;
    unsigned y_first = g->agg_2hop_keys[base_idx+x];
    unsigned y_last = g->agg_2hop_keys[base_idx+x+1];
    unsigned *x_first = g->al_crs + g->al_keys[x_idx];
    unsigned *x_last = g->al_crs + g->al_keys[x_idx+1];
    
    int match_num = 0;

    while (x_first != x_last && y_first != y_last) {
        if (*x_first < g->agg_2hop_crs[y_first]) {
            ++x_first;
        }
        else if (*x_first > g->agg_2hop_crs[y_first]) {
            ++y_first;
        }
        else {
            intersects[match_num] = y_first;
            ++match_num;
            ++x_first;
            ++y_first;
        }
    }

    unsigned num_results = 0;
    for (int i=0; i<match_num; ++i) {
        unsigned *first = g->agg_al_crs + g->agg_al_keys[intersects[i]];
        unsigned *last = g->agg_al_crs + g->agg_al_keys[intersects[i]+1];
        unsigned z = g->agg_2hop_crs[intersects[i]];

        while (first != last) {
            assignment = {x, *first, z};
            ++num_results;
            ++first;
        }
    }

    return num_results;
}
