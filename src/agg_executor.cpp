#include "../include/executor_new.hpp"


void AggExecutor::init() {
    std::unordered_map<int,int> order_inv;
    for (int i=0; i<order.size(); i++) { order_inv[order[i]] = i; }
    // scanを行うエッジの方向・ラベル, intersectionを行う隣接リストの方向・ラベルの特定
    std::unordered_map<std::string, int> meta;

    // need some changes
    es.resize(3);
    unsigned base_base;
    int sr, ib, edl, dsl;

    for (auto &triple : q->triples) {
        int src = order_inv[triple[0]];
        int el = triple[1];
        int dst = order_inv[triple[2]];
        int sl = q->vars[triple[0]];
        int dl = q->vars[triple[2]];
        int is_bwd = false;
        if (src > dst) {
            int tmp = src;
            src = dst; dst = tmp;
            tmp = sl;
            sl = dl; dl = tmp;
            is_bwd = true;
        }

        if (src == 0 && dst == 1) {
            scan_vl = sl;
            es[0] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (
                is_bwd * g->graph_info["num_e_labels"] + el) + dl), src };
        }
        else if (src == 0 && dst == 2) {
            base_base = g->graph_info["num_v_labels"] * (is_bwd * g->graph_info["num_e_labels"] + el) + dl;
            sr = src;
        }
        else if (src == 1 && dst == 3) {
            es[1] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (
                is_bwd * g->graph_info["num_e_labels"] + el) + dl), src };
        }
        else {
            ib = is_bwd; edl = el; dsl = dl;
        }
    }
    es[2] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * (2 * base_base + ib) + edl) + dsl), sr };

    // 各種変数の初期化
    intersects = new unsigned[g->graph_info["max_degree"]];
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
    unsigned first = g->scan_keys[scan_vl];
    unsigned last = g->scan_keys[scan_vl+1];

    unsigned result_size = 0;
    unsigned intersection_count = 0;

    for (int i=first; i<last; ++i) {
        unsigned x = g->scan_crs[i];
        unsigned idx0 = es[0].first + x;
        unsigned j_first = g->al_keys[idx0];
        unsigned j_last = g->al_keys[idx0+1];
        for (int j=j_first; j<j_last; ++j) {
            unsigned y = g->al_crs[j];
            ++intersection_count;
            int match_num = intersect_agg(es[1].first+y, es[2].first+y);
            for (int k=0; k<match_num; ++k) {
                unsigned z = intersects[k];
                unsigned l_first = g->agg_al_keys[z];
                unsigned l_last = g->agg_al_keys[z+1];
                for (int l=l_first; l<l_last; ++l) {
                    unsigned w = g->agg_2hop_crs[l];
                    ++result_size;
                }
            }
        }
    }

    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void AggExecutor::hybrid_join() {
    // unsigned first = g->scan_keys[scan_idx];
    // unsigned last = g->scan_keys[scan_idx+1];

    // unsigned result_size = 0;
    // unsigned intersection_count = 0;

    // for (int i=first; i<last; ++i) {
    //     unsigned dst_first = g->scan_crs[i];
    //     unsigned dst_last = g->scan_crs[i+1];
    //     for (int j=dst_first; j<dst_last; ++j) {
    //         ++intersection_count;
    //         int num_results = intersect(g->scan_srcs[i], g->scan_dst_crs[j]);
    //         for (int k=0; k<num_results; ++k) {
    //             assignment = {g->scan_srcs[i], g->scan_dst_crs[j], intersects[k]};
    //             ++result_size;
    //         }
    //     }
    //     ++intersection_count;
    //     unsigned num_results = intersect_agg(g->scan_srcs[i]);
    //     result_size += num_results;
    // }
    // exec_stats["intersection_count"] = intersection_count;
    // exec_stats["result_size"] = result_size;
}


int AggExecutor::intersect_agg(unsigned x, unsigned y) {
    unsigned y_first = g->agg_2hop_keys[y];
    unsigned y_last = g->agg_2hop_keys[y+1];
    unsigned *x_first = g->al_crs + g->al_keys[x];
    unsigned *x_last = g->al_crs + g->al_keys[x+1];
    
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

    return match_num;
}
