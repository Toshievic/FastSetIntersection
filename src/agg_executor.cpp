#include "../include/executor.hpp"

#include <cstring>


void AggExecutor::init() {
    // 各種変数の初期化
    v_first = order[0];
    assignment = new unsigned[order.size()];
    std::vector<std::vector<std::tuple<int,int,int,int>>> tmp_plan(order.size()-1);

    // クエリと与えられたorderから各depthでどの隣接リストを参照し, intersectionを行うのか特定
    scan_vl = q->vars[order[0]];
    std::unordered_map<int,int> order_inv;
    for (int i=0; i<order.size(); i++) { order_inv[order[i]] = i; }
    for (auto &triple : q->triples) {
        int src = triple[0];
        int el = triple[1];
        int dst = triple[2];
        int sl = q->vars[triple[0]];
        int dl = q->vars[triple[2]];
        int is_bwd = false;
        if (order_inv[src] > order_inv[dst]) {
            int tmp = src;
            src = dst; dst = tmp;
            tmp = sl;
            sl = dl; dl = tmp;
            is_bwd = true;
        }
        // unsigned key = g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * g->graph_info["num_e_labels"] + el) + dl);
        tmp_plan[order_inv[dst]-1].push_back({is_bwd, el, dl, src});
    }

    general_plan.resize(tmp_plan.size());
    // general planを集約隣接リストを用いたplanに置き換え
    std::unordered_map<int,int> replaced;
    std::vector<int> tmp_agg_order;
    std::vector<std::vector<int>> tmp_agg_detail(tmp_plan.size());
    for (int i=0; i<tmp_plan.size(); ++i) { // i ... 2-hop先頂点
        tmp_agg_order.push_back(order[i+1]);
        general_plan[i].resize(tmp_plan[i].size());
        for (int j=0; j<tmp_plan[i].size(); ++j) {
            auto [is_bwd, el, dl, src] = tmp_plan[i][j]; // src ... 経由頂点
            general_plan[i][j] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * 
                g->graph_info["num_e_labels"] + el) + dl), src};
            if (tmp_plan[i].size() > 1) {
                if (order_inv[src] != 0 && tmp_plan[order_inv[src]-1].size() < 2 && !replaced.contains(src)) {
                    replaced[src] = order[i+1];
                    tmp_agg_order.erase(std::remove(tmp_agg_order.begin(), tmp_agg_order.end(), src), tmp_agg_order.end());
                    tmp_agg_detail[i].push_back(src);
                }
            }
        }
    }
    agg_order.resize(tmp_agg_order.size());
    agg_plan.resize(tmp_agg_order.size());
    agg_detail.resize(tmp_agg_order.size());
    for (int i=0; i<tmp_agg_order.size(); ++i) {
        agg_order[i] = tmp_agg_order[i];
        int idx = order_inv[agg_order[i]]-1;
        agg_plan[i].resize(general_plan[idx].size());
        agg_detail[i].resize(tmp_agg_detail[idx].size());
        for (int j=0; j<tmp_agg_detail[idx].size(); ++j) {
            agg_detail[i][j] = tmp_agg_detail[idx][j];
        }
        int itr = 0;
        for (int j=0; j<general_plan[idx].size(); ++j) {
            if (!replaced.contains(general_plan[idx][j].second) || replaced[general_plan[idx][j].second] != agg_order[i]) {
                agg_plan[i][itr] = general_plan[idx][j];
                ++itr;
            }
        }
        for (int j=0; j<general_plan[idx].size(); ++j) {
            if (replaced.contains(general_plan[idx][j].second) && replaced[general_plan[idx][j].second] == agg_order[i]) {
                auto [is_bwd1, el1, dl1, src1] = tmp_plan[idx][j];
                auto [is_bwd0, el0, dl0, src] = tmp_plan[order_inv[src1]-1][0];
                agg_plan[i][itr] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * (2 * (
                    g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * is_bwd0 + el0) + dl0) + is_bwd1) + el1) + dl1),
                    src};
                ++itr;
            }
        }
    }

    intersect_store.resize(agg_plan.size());
    match_nums.resize(agg_plan.size());
    ptr_store.resize(agg_plan.size());
    for (int i=0; i<agg_plan.size(); ++i) {
        intersect_store[i].resize(agg_plan[i].size());
        match_nums[i].resize(agg_plan[i].size());
        ptr_store[i].resize(agg_plan[i].size());
        int num_normal = agg_plan[i].size() - agg_detail[i].size();
        for (int j=0; j<agg_plan[i].size(); ++j) {
            intersect_store[i][j] = new unsigned[g->graph_info["num_v"]];
            match_nums[i][j] = 0;
            if (j >= num_normal) {
                int segments = std::min((int)agg_detail[i].size(), j+1);
                ptr_store[i][j].resize(segments);
                for (int k=0; k<segments; ++k) {
                    ptr_store[i][j][k] = new unsigned[g->graph_info["num_v"]];
                }
            }
        }
    }
}


void AggExecutor::run(std::unordered_map<std::string, std::string> &options) {
    Chrono_t start = get_time();
    join();
    Chrono_t end = get_time();
    time_stats["join_runtime"] = get_runtime(&start, &end);
    summarize_result(options);
}


void AggExecutor::join() {
    unsigned first = g->scan_keys[scan_vl];
    unsigned last = g->scan_keys[scan_vl+1];

    for (int i=first; i<last; ++i) {
        assignment[v_first] = i;
        recursive_join(0);
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void AggExecutor::recursive_join(int depth) {
    int normals = agg_plan[depth].size() - agg_detail[depth].size();
    int intersects = agg_plan[depth].size();
    int processed_aggs = 0;

    unsigned idx = agg_plan[depth][0].first + assignment[agg_plan[depth][0].second];
    unsigned *first, *last;
    if (normals > 0) {
        first = g->al_crs + g->al_keys[idx];
        last = g->al_crs + g->al_keys[idx+1];
    }
    else {
        unsigned _first = g->agg_2hop_keys[idx];
        unsigned _last = g->agg_2hop_keys[idx+1];
        first = g->agg_2hop_crs + _first;
        last = g->agg_2hop_crs + _last;
        for (int i=0; i<_last-_first; ++i) {
            ptr_store[depth][0][0][i] = _first+i;
        }
        normals = 1;
        ++processed_aggs;
    }
    match_nums[depth][0] = last - first;
    std::memcpy(intersect_store[depth][0], first, sizeof(unsigned)*(last-first));

    for (int i=1; i<normals; ++i) {
        ++intersection_count;
        unsigned *x_first = intersect_store[depth][i-1];
        unsigned *x_last = intersect_store[depth][i-1] + match_nums[depth][i-1];
        unsigned y_idx = agg_plan[depth][i].first + assignment[agg_plan[depth][i].second];
        unsigned *y_first = g->al_crs + g->al_keys[y_idx];
        unsigned *y_last = g->al_crs + g->al_keys[y_idx+1];

        int match_num = 0;
        while (x_first != x_last && y_first != y_last) {
            if (*x_first < *y_first) {
                ++x_first;
            }
            else if (*x_first > *y_first) {
                ++y_first;
            }
            else {
                intersect_store[depth][i][match_num] = *x_first;
                ++match_num;
                ++x_first;
                ++y_first;
            }
        }
        match_nums[depth][i] = match_num;
    }
    for (int i=normals; i<intersects; ++i) {
        ++intersection_count;
        unsigned x_first = 0;
        int x_last = match_nums[depth][i-1];
        // unsigned *x_first = intersect_store[depth][i-1];
        unsigned y_idx = agg_plan[depth][i].first + assignment[agg_plan[depth][i].second];
        unsigned y_first = g->agg_2hop_keys[y_idx];
        unsigned y_last = g->agg_2hop_keys[y_idx+1];

        int match_num = 0;
        while (x_first != x_last && y_first != y_last) {
            if (intersect_store[depth][i-1][x_first] < g->agg_2hop_crs[y_first]) {
                ++x_first;
            }
            else if (intersect_store[depth][i-1][x_first] > g->agg_2hop_crs[y_first]) {
                ++y_first;
            }
            else {
                intersect_store[depth][i][match_num] = g->agg_2hop_crs[y_first];
                ptr_store[depth][i][processed_aggs][match_num] = y_first;
                for (int j=0; j<processed_aggs; ++j) {
                    ptr_store[depth][i][j][match_num] = ptr_store[depth][i-1][j][x_first];
                }
                ++match_num;
                ++x_first;
                ++y_first;
            }
        }
        match_nums[depth][i] = match_num;
        ++processed_aggs;
    }

    unsigned *intersect = intersect_store[depth][intersects-1];
    std::vector<unsigned *> ptrs = ptr_store[depth][intersects-1];
    int match_num = match_nums[depth][intersects-1];
    for (int i=0; i<match_num; ++i) {
        assignment[agg_order[depth]] = intersect[i];
        if (ptrs.size() > 0) {
            unsigned first_0 = g->agg_al_keys[ptrs[0][i]];
            unsigned last_0 = g->agg_al_keys[ptrs[0][i]+1];
            for (int j=first_0; j<last_0; ++j) {
                assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                if (ptrs.size() > 1) {
                    unsigned first_1 = g->agg_al_keys[ptrs[1][i]];
                    unsigned last_1 = g->agg_al_keys[ptrs[1][i]+1];
                    for (int k=first_1; k<last_1; ++k) {
                        assignment[agg_detail[depth][1]] = g->agg_al_crs[k];
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else { recursive_join(depth+1); }
                    }
                }
                else {
                    if (depth == agg_order.size()-1) { ++result_size; }
                    else { recursive_join(depth+1); }
                }
            }
        }
        else {
            if (depth == agg_order.size()-1) { ++result_size; }
            else { recursive_join(depth+1); }
        }
    }
}
