#include "../include/executor_new.hpp"


void SimpleGenericExecutor::init() {
    std::unordered_map<int,int> order_inv;
    for (int i=0; i<order.size(); i++) { order_inv[order[i]] = i; }
    // scanを行うエッジの方向・ラベル, intersectionを行う隣接リストの方向・ラベルの特定
    std::unordered_map<std::string, int> meta;

    // need some changes
    es.resize(2);

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
            es[1] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (
                is_bwd * g->graph_info["num_e_labels"] + el) + dl), src };
        }
        else {
            es[2] = { g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (
                is_bwd * g->graph_info["num_e_labels"] + el) + dl), src };
        }
    }

    // 各種変数の初期化
    intersects = new unsigned[g->graph_info["max_degree"]];
}


void SimpleGenericExecutor::run(std::unordered_map<std::string, std::string> &options) {
    Chrono_t start = get_time();
    // options:
    // scan=factorize であれば, 0番目の頂点の更新を抑制
    // switch=lazy であれば, ポインタの更新を抑制
    if (options.contains("scan") && options["scan"] == "factorize") {
        factorize_join();
    }
    else if (options.contains("switch") && options["switch"] == "lazy") {
        join_with_lazy_update();
    }
    else {
        join();
    }
    Chrono_t end = get_time();
    time_stats["join_runtime"] = get_runtime(&start, &end);
    summarize_result();
}


void SimpleGenericExecutor::join() {
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
            int match_num = intersect(es[1].first+x, es[2].first+y);
            for (int k=0; k<match_num; ++k) {
                unsigned z = intersects[k];
                ++result_size;
            }
        }
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


int SimpleGenericExecutor::intersect(unsigned x, unsigned y) {
    unsigned *x_first = g->al_crs + g->al_keys[x];
    unsigned *x_last = g->al_crs + g->al_keys[x+1];
    unsigned *y_first = g->al_crs + g->al_keys[y];
    unsigned *y_last = g->al_crs + g->al_keys[y+1];

    int match_num = 0;

    while (x_first != x_last && y_first != y_last) {
        if (*x_first < *y_first) {
            ++x_first;
        }
        else if (*x_first > *y_first) {
            ++y_first;
        }
        else {
            intersects[match_num] = *x_first;
            ++match_num;
            ++x_first;
            ++y_first;
        }
    }

    return match_num;
}


void SimpleGenericExecutor::factorize_join() {
    // // 0番目の頂点が更新されない限り, 読み込みを発生させない
    // unsigned first = g->scan_keys[scan_idx];
    // unsigned last = g->scan_keys[scan_idx+1];

    // unsigned last_src = g->scan_crs[0];
    // unsigned x_idx = x_base_idx + last_src;
    // unsigned *x_first = g->al_crs + g->al_keys[x_idx];
    // unsigned *x_last = g->al_crs + g->al_keys[x_idx+1];

    // unsigned result_size = 0;
    // unsigned intersection_count = 0;

    // for (int i=first; i<last; ++i) {
    //     ++intersection_count;
    //     if (g->scan_crs[i] != last_src) {
    //         last_src = g->scan_crs[i];
    //         unsigned x_idx = x_base_idx + last_src;
    //         x_first = g->al_crs + g->al_keys[x_idx];
    //         x_last = g->al_crs + g->al_keys[x_idx+1];
    //     }
    //     int num_results = intersect_for_factorize(x_first, x_last, g->scan_dst_crs[i]);
    //     for (int j=0; j<num_results; ++j) {
    //         ++result_size;
    //     }
    // }
    // exec_stats["intersection_count"] = intersection_count;
    // exec_stats["result_size"] = result_size;
}


int SimpleGenericExecutor::intersect_for_factorize(unsigned *x_first, unsigned *x_last, unsigned y) {
    // unsigned y_idx = y_base_idx + y;
    // unsigned *y_first = g->al_crs + g->al_keys[y_idx];
    // unsigned *y_last = g->al_crs + g->al_keys[y_idx+1];

    int match_num = 0;

    // while (x_first != x_last && y_first != y_last) {
    //     if (*x_first < *y_first) {
    //         ++x_first;
    //     }
    //     else if (*x_first > *y_first) {
    //         ++y_first;
    //     }
    //     else {
    //         intersects[match_num] = *x_first;
    //         ++match_num;
    //         ++x_first;
    //         ++y_first;
    //     }
    // }

    return match_num;
}


void SimpleGenericExecutor::join_with_lazy_update() {
    // unsigned first = g->scan_keys[scan_idx];
    // unsigned last = g->scan_keys[scan_idx+1];

    // unsigned result_size = 0;
    // unsigned intersection_count = 0;

    // for (int i=first; i<last; ++i) {
    //     ++intersection_count;
    //     int num_results = intersect_lazy_update(g->scan_crs[i], g->scan_dst_crs[i]);
    //     for (int j=0; j<num_results; ++j) {
    //         ++result_size;
    //     }
    // }
    // exec_stats["intersection_count"] = intersection_count;
    // exec_stats["result_size"] = result_size;
}


int SimpleGenericExecutor::intersect_lazy_update(unsigned x, unsigned y) {
    // unsigned x_idx = x_base_idx + x;
    // unsigned y_idx = y_base_idx + y;
    // unsigned *x_first = g->al_crs + g->al_keys[x_idx];
    // unsigned *x_last = g->al_crs + g->al_keys[x_idx+1];
    // unsigned *y_first = g->al_crs + g->al_keys[y_idx];
    // unsigned *y_last = g->al_crs + g->al_keys[y_idx+1];

    int match_num = 0;

    // while (x_first != x_last && y_first != y_last) {
    //     if (*x_first < *y_first) {
    //         ++x_first;
    //         while (x_first != x_last && *x_first < *y_first) {
    //             ++x_first;
    //         }
    //     }
    //     else if (*x_first > *y_first) {
    //         ++y_first;
    //         while (y_first != y_last && *x_first > *y_first) {
    //             ++y_first;
    //         }
    //     }
    //     else {
    //         intersects[match_num] = *x_first;
    //         ++match_num;
    //         ++x_first;
    //         ++y_first;
    //     }
    // }

    return match_num;
}


void SimpleGenericExecutor::summarize_result() {
    using namespace std;
    cout << "--- Basic Info ---" << endl;
    cout << "Method: " << executor_name << "\t\tQuery: " << q->query_name << endl;
    cout << "--- Runtimes ---" << endl;
    for (auto &item : time_stats) {
        cout << item.first << ": " << to_string(item.second) << " [ms]" << endl;
    }
    cout << "--- Results ---" << endl;
    for (auto &item : exec_stats) {
        cout << item.first << ": " << to_string(item.second) << endl;
    }
    cout << endl;
}
