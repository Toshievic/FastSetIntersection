#include "../include/executor_new.hpp"


void AggExecutor::init() {
    std::unordered_map<int,int> order_inv;
    for (int i=0; i<order.size(); i++) { order_inv[order[i]] = i; }
    // scanを行うエッジの方向・ラベル, intersectionを行う隣接リストの方向・ラベルの特定
    std::unordered_map<std::string, int> meta;

    // need some changes
    es.resize(2);
    unsigned base_base1, base_base2;
    int sr0, ib0, edl0, dsl0;
    int sr1, ib1, edl1, dsl1;

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
            base_base1 = g->graph_info["num_v_labels"] * (is_bwd * g->graph_info["num_e_labels"] + el) + dl;
            sr0 = src;
        }
        else if (src == 0 && dst == 2) {
            base_base2 = g->graph_info["num_v_labels"] * (is_bwd * g->graph_info["num_e_labels"] + el) + dl;
            sr1 = src;
        }
        else if (src == 1 && dst == 3) {
            ib0 = is_bwd; edl0 = el; dsl0 = dl;
        }
        else {
            ib1 = is_bwd; edl1 = el; dsl1 = dl;
        }
    }
    es[0] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * (2 * base_base1 + ib0) + edl0) + dsl0), sr0 };
    es[1] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * (2 * base_base2 + ib1) + edl1) + dsl1), sr1 };

    // 各種変数の初期化
    intersects = new unsigned[g->graph_info["max_degree"]];
    intersects_sub = new unsigned[g->graph_info["max_degree"]];
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

    unsigned long long result_size = 0;
    unsigned long long intersection_count = 0;

    for (int i=first; i<last; ++i) {
        unsigned x = g->scan_crs[i];
        ++intersection_count;
        int match_num = intersect_wagg(es[0].first+x, es[1].first+x);
        for (int j=0; j<match_num; ++j) {
            unsigned y0 = intersects[j];
            unsigned y1 = intersects_sub[j];
            unsigned k_first = g->agg_al_keys[y0];
            unsigned k_last = g->agg_al_keys[y0+1];
            unsigned l_first = g->agg_al_keys[y1];
            unsigned l_last = g->agg_al_keys[y1+1];
            for (int k=k_first; k<k_last; ++k) {
                unsigned z = g->agg_2hop_crs[k];
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


int AggExecutor::intersect_wagg(unsigned x, unsigned y) {
    unsigned y_first = g->agg_2hop_keys[y];
    unsigned y_last = g->agg_2hop_keys[y+1];
    unsigned x_first = g->agg_2hop_keys[x];
    unsigned x_last = g->agg_2hop_keys[x+1];
    
    int match_num = 0;

    while (x_first != x_last && y_first != y_last) {
        if (g->agg_2hop_crs[x_first] < g->agg_2hop_crs[y_first]) {
            ++x_first;
        }
        else if (g->agg_2hop_crs[x_first] > g->agg_2hop_crs[y_first]) {
            ++y_first;
        }
        else {
            intersects[match_num] = x_first;
            intersects_sub[match_num] = y_first;
            ++match_num;
            ++x_first;
            ++y_first;
        }
    }

    return match_num;
}
