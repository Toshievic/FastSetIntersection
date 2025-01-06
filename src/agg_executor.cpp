#include "../include/executor.hpp"

#include <cstring>
#include <algorithm>


void AggExecutor::init() {
    // 各種変数の初期化
    v_first = order[0];
    assignment = new unsigned[order.size()];
    std::vector<std::vector<std::tuple<int,int,int,int>>> tmp_plan(order.size()-1);

    // tmp_planを作成
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
        tmp_plan[order_inv[dst]-1].push_back({is_bwd, el, dl, src});
    }

    // 集約隣接リストを使用するタイミングを特定, 入替判定も行う
    std::unordered_map<int,int> replaced; // 経由 -> 2-hop先, 入替後
    std::unordered_map<int,int> replaced_original; // 経由 -> 2-hop先, 入替前
    std::vector<std::vector<int>> replaced_inv(order.size()); // 2-hop先 -> 経由, 入替後
    std::vector<std::vector<int>>replaced_inv_original(order.size()); // 2-hop先 -> 経由, 入替前
    std::vector<int> changed_order = order; // 入替後の順序を保持

    for (int i=0; i<tmp_plan.size(); ++i) {
        for (int j=0; j<tmp_plan[i].size(); ++j) {
            auto [is_bwd, el, dl, src] = tmp_plan[i][j]; // src ... 隣接元頂点
            if (tmp_plan[i].size() > 1) {
                // 集約隣接リストの置き換え条件
                if (order_inv[src] != 0 && tmp_plan[order_inv[src]-1].size() < 2 && !replaced.contains(src) && replaced_inv[src].size() == 0) {
                    // 入替判定
                    bool should_change = false;
                    for (int k=i+1; k<tmp_plan.size(); ++k) {
                        for (int l=0; l<tmp_plan[k].size(); ++l) {
                            auto [_is_bwd, _el, _dl, _src] = tmp_plan[k][l];
                            // 経由頂点が以降のintersectionで用いられるならば
                            // if (src == _src) { should_change = true; }
                        }
                    }
                    if (should_change) { // この場合はorder[i+1]が経由, srcが2-hop先
                        replaced[order[i+1]] = src;
                        replaced_inv[src].push_back(order[i+1]);
                        changed_order[i+1] = src;
                        changed_order[order_inv[src]] = order[i+1];
                        replaced_original[order[i+1]] = src;
                        replaced_inv_original[src].push_back(order[i+1]);
                    }
                    else { // この場合はsrcが経由, order[i+1]が2-hop先
                        replaced[src] = order[i+1];
                        replaced_inv[order[i+1]].push_back(src);
                        replaced_original[src] = order[i+1];
                        replaced_inv_original[order[i+1]].push_back(src);
                    }
                }
            }
        }
    }

    // 入替後バージョンのtmp_planを作成
    std::vector<std::vector<std::tuple<int,int,int,int>>> changed_plan(order.size()-1);
    std::unordered_map<int,int> changed_order_inv;
    std::vector<int> assign_order(order.size()); // キャッシュ用のorder, 集約隣接リストの使用による割り当て順序の逆転を考慮
    std::unordered_map<int,int> assign_order_inv;
    int _itr = 0;
    for (int i=0; i<changed_order.size(); i++) {
        changed_order_inv[changed_order[i]] = i;
        if (!replaced.contains(changed_order[i])) {
            assign_order[_itr] = changed_order[i];
            assign_order_inv[changed_order[i]] = _itr;
            ++_itr;
            for (int j=0; j<replaced_inv[changed_order[i]].size(); ++j) {
                assign_order[_itr] = replaced_inv[changed_order[i]][j];
                assign_order_inv[replaced_inv[changed_order[i]][j]] = _itr;
                ++_itr;
            }
        }
    }
    for (auto &triple : q->triples) {
        int src = triple[0];
        int el = triple[1];
        int dst = triple[2];
        int sl = q->vars[triple[0]];
        int dl = q->vars[triple[2]];
        int is_bwd = false;
        if (changed_order_inv[src] > changed_order_inv[dst]) {
            int tmp = src;
            src = dst; dst = tmp;
            tmp = sl;
            sl = dl; dl = tmp;
            is_bwd = true;
        }
        changed_plan[changed_order_inv[dst]-1].push_back({is_bwd, el, dl, src});
    }

    // order, plan, detailを作成
    general_order.resize(tmp_plan.size()-replaced.size());
    agg_order.resize(tmp_plan.size()-replaced.size());
    general_detail.resize(tmp_plan.size()-replaced.size());
    agg_detail.resize(tmp_plan.size()-replaced.size());
    general_plan.resize(tmp_plan.size()-replaced.size());
    agg_plan.resize(tmp_plan.size()-replaced.size());
    int itr_g = 0; int itr_a = 0;
    for (int i=0; i<order.size()-1; ++i) {
        if (!replaced_original.contains(changed_order[i+1])) {
            general_order[itr_g] = changed_order[i+1];
            general_detail[itr_g].resize(replaced_inv_original[changed_order[i+1]].size());
            general_detail[itr_g].assign(replaced_inv_original[changed_order[i+1]].begin(), replaced_inv_original[changed_order[i+1]].end());
            general_plan[itr_g].resize(replaced_inv_original[changed_order[i+1]].size()+1);
            for (int j=0; j<replaced_inv_original[changed_order[i+1]].size(); ++j) {
                int via = replaced_inv_original[changed_order[i+1]][j];
                general_plan[itr_g][j].resize(changed_plan[changed_order_inv[via]-1].size());
                for (int k=0; k<changed_plan[changed_order_inv[via]-1].size(); ++k) {
                    auto [is_bwd, el, dl, src] = changed_plan[changed_order_inv[via]-1][k];
                    general_plan[itr_g][j][k] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * 
                        g->graph_info["num_e_labels"] + el) + dl), src};
                }
            }
            // 集約隣接リストを使用しないものから先にplanへ入れる
            int j = replaced_inv_original[changed_order[i+1]].size();
            int itr = 0;
            general_plan[itr_g][j].resize(changed_plan[i].size());
            for (int k=0; k<changed_plan[i].size(); ++k) {
                auto [is_bwd, el, dl, src] = changed_plan[i][k];
                if (std::find(replaced_inv_original[changed_order[i+1]].begin(), replaced_inv_original[changed_order[i+1]].end(), src) ==
                replaced_inv_original[changed_order[i+1]].end()) {
                    general_plan[itr_g][j][itr] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * 
                        g->graph_info["num_e_labels"] + el) + dl), src};
                    ++itr;
                }
            }
            for (int k=0; k<tmp_plan[i].size(); ++k) {
                auto [is_bwd, el, dl, src] = changed_plan[i][k];
                if (std::find(replaced_inv_original[changed_order[i+1]].begin(), replaced_inv_original[changed_order[i+1]].end(), src) !=
                replaced_inv_original[changed_order[i+1]].end()) {
                    general_plan[itr_g][j][itr] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * 
                        g->graph_info["num_e_labels"] + el) + dl), src};
                    ++itr;
                }
            }
            ++itr_g;
        }
        if (!replaced.contains(changed_order[i+1])) {
            agg_order[itr_a] = changed_order[i+1];
            agg_detail[itr_a].resize(replaced_inv[changed_order[i+1]].size());
            agg_detail[itr_a].assign(replaced_inv[changed_order[i+1]].begin(), replaced_inv[changed_order[i+1]].end());
            agg_plan[itr_a].resize(changed_plan[i].size());
            // 集約隣接リストを使用しないものから先にplanへ入れる
            int itr = 0;
            for (int j=0; j<changed_plan[i].size(); ++j) {
                auto [is_bwd, el, dl, src] = changed_plan[i][j];
                if (std::find(replaced_inv[changed_order[i+1]].begin(), replaced_inv[changed_order[i+1]].end(), src) ==
                replaced_inv[changed_order[i+1]].end()) {
                    agg_plan[itr_a][itr] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * 
                        g->graph_info["num_e_labels"] + el) + dl), src};
                    ++itr;
                }
            }
            for (int j=0; j<changed_plan[i].size(); ++j) {
                auto [is_bwd1, el1, dl1, src1] = changed_plan[i][j];
                if (std::find(replaced_inv[changed_order[i+1]].begin(), replaced_inv[changed_order[i+1]].end(), src1) !=
                replaced_inv[changed_order[i+1]].end()) {
                    // 集約隣接リスト専用のkeyを作成
                    auto [is_bwd0, el0, dl0, src0] = changed_plan[changed_order_inv[src1]-1][0];
                    agg_plan[itr_a][itr] = {g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * (2 * (
                        g->graph_info["num_v_labels"] * (g->graph_info["num_e_labels"] * is_bwd0 + el0) + dl0) + is_bwd1) + el1) + dl1), src0};
                    ++itr;
                }
            }
            ++itr_a;
        }
    }

    intersect_store.resize(agg_plan.size());
    match_nums.resize(agg_plan.size());
    ptr_store.resize(agg_plan.size());
    for (int i=0; i<agg_plan.size(); ++i) {
        if (general_plan[i].size() == 1) { general_plan[i].clear(); }
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

    // cache setup
    cache_reset.resize(order.size());
    start_from.resize(agg_plan.size(), 0);
    cache_set.resize(agg_plan.size(), 0);
    for (int i=0; i<agg_plan.size(); ++i) {
        for (int j=0; j<agg_plan[i].size(); ++j) {
            // agg_plan[i]内をキャッシュが効率的に使えるようにソート
            std::sort(agg_plan[i].begin(), agg_plan[i].begin()+agg_plan[i].size()-agg_detail[i].size(),
            [&assign_order_inv](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                return assign_order_inv[a.second] < assign_order_inv[b.second];
            });
            int src = agg_plan[i][j].second;
            int src_order = std::distance(assign_order.begin(), std::find(assign_order.begin(), assign_order.end(), src));
            int dst_order = std::distance(assign_order.begin(), std::find(assign_order.begin(), assign_order.end(), agg_order[i]));
            if (dst_order - src_order > 1) {
                cache_set[i] = j+1;
                cache_reset[src].push_back({i,j});
            }
        }
    }

    result_size = 0;
    intersection_count = 0;
}


void AggExecutor::run(std::unordered_map<std::string, std::string> &options) {
    Chrono_t start = get_time();
    if (options.contains("cache") && options["cache"]=="on") {
        cache_join();
    }
    else {
        join();
    }
    Chrono_t end = get_time();
    time_stats["join_runtime"] = get_runtime(&start, &end);
    summarize_result(options);
}


void AggExecutor::join() {
    unsigned first = g->scan_keys[scan_vl];
    unsigned last = g->scan_keys[scan_vl+1];
    std::vector<int> vec;

    for (int i=first; i<last; ++i) {
        assignment[v_first] = i;
        recursive_agg_join(0, 0, vec);
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void AggExecutor::cache_join() {
    unsigned first = g->scan_keys[scan_vl];
    unsigned last = g->scan_keys[scan_vl+1];
    std::vector<int> vec;

    for (int i=first; i<last; ++i) {
        assignment[v_first] = i;
        for (int j=0; j<cache_reset[v_first].size(); ++j) {
            start_from[cache_reset[v_first][j].first] = cache_reset[v_first][j].second;
        }
        recursive_agg_cache_join(0, start_from[0], vec);
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void AggExecutor::recursive_agg_join(int depth, int stage, std::vector<int> &use_agg) {
    // use_agg: どのstageで集約隣接リストを使用したのか
    if (agg_plan[depth].size() - agg_detail[depth].size() > stage) {
        // normal mode, 普通に隣接リストをロードしていく
        unsigned idx = agg_plan[depth][stage].first + assignment[agg_plan[depth][stage].second];
        unsigned *first = g->al_crs + g->al_keys[idx];
        unsigned *last = g->al_crs + g->al_keys[idx+1];
        if (stage == 0) {
            match_nums[depth][0] = last - first;
            std::memcpy(intersect_store[depth][0], first, sizeof(unsigned)*(last-first));
            if (agg_plan[depth].size() == 1) {
                while (first != last) {
                    assignment[agg_order[depth]] = *first;
                    if (depth == agg_plan.size() - 1) { ++result_size; }
                    else {
                        std::vector<int> new_vec;
                        recursive_agg_join(depth+1, 0, new_vec);
                    }
                    ++first;
                }
            }
            else { recursive_agg_join(depth, stage+1, use_agg); }
        }
        else {
            // intersectionを行うモード
            ++intersection_count;
            unsigned x_first = 0;
            unsigned x_last = match_nums[depth][stage-1];

            int match_num = 0;
            while (x_first != x_last && first != last) {
                if (intersect_store[depth][stage-1][x_first] < *first) { ++x_first; }
                else if (intersect_store[depth][stage-1][x_first] > *first) { ++first; }
                else {
                    intersect_store[depth][stage][match_num] = *first;
                    for (int j=0; j<use_agg.size(); ++j) {
                        ptr_store[depth][stage][use_agg[j]][match_num] = ptr_store[depth][stage-1][use_agg[j]][x_first];
                    }
                    ++match_num; ++x_first; ++first;
                }
            }
            match_nums[depth][stage] = match_num;

            if (stage == agg_plan[depth].size()-1) {
                unsigned *intersect = intersect_store[depth][stage];
                int match_num = match_nums[depth][stage];
                for (int i=0; i<match_num; ++i) {
                    assignment[agg_order[depth]] = intersect[i];
                    if (use_agg.size() > 0) {
                        unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]];
                        unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]+1];
                        for (int j=first_0; j<last_0; ++j) {
                            assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                            if (use_agg.size() > 1) {
                                unsigned first_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]];
                                unsigned last_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]+1];
                                for (int k=first_1; k<last_1; ++k) {
                                    assignment[agg_detail[depth][1]] = g->agg_al_crs[k];
                                    if (depth == agg_order.size()-1) { ++result_size; }
                                    else {
                                        std::vector<int> new_vec;
                                        recursive_agg_join(depth+1, 0, new_vec);
                                    }
                                }
                            }
                            else {
                                if (depth == agg_order.size()-1) { ++result_size; }
                                else {
                                    std::vector<int> new_vec;
                                    recursive_agg_join(depth+1, 0, new_vec);
                                }
                            }
                        }
                    }
                    else {
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_join(depth+1, 0, new_vec);
                        }
                    }
                }
            }
            else { recursive_agg_join(depth, stage+1, use_agg); }
        }
    }
    else {
        // agg_mode, 集約隣接リスト, ハブリストを使う
        int hub_id = stage+agg_detail[depth].size()-agg_plan[depth].size();
        unsigned agg_idx = agg_plan[depth][stage].first + assignment[agg_plan[depth][stage].second];
        unsigned hub_idx = general_plan[depth][hub_id][0].first + assignment[general_plan[depth][hub_id][0].second];
        unsigned _agg_first = g->agg_2hop_keys[agg_idx]; unsigned _agg_last = g->agg_2hop_keys[agg_idx+1];
        unsigned *agg_first = g->agg_2hop_crs + _agg_first; unsigned *agg_last = g->agg_2hop_crs + _agg_last;
        unsigned *hub_first = g->al_hub_crs + g->al_hub_keys[hub_idx]; unsigned *hub_last = g->al_hub_crs + g->al_hub_keys[hub_idx+1];

        if (stage == 0) {
            match_nums[depth][0] = agg_last - agg_first;
            std::memcpy(intersect_store[depth][0], agg_first, sizeof(unsigned)*(agg_last-agg_first));
            for (int i=0; i<_agg_last-_agg_first; ++i) {
                ptr_store[depth][0][0][i] = _agg_first+i;
            }
            if (agg_plan[depth].size() == 1) {
                for (int i=0; i<_agg_last-_agg_first; ++i) {
                    assignment[agg_order[depth]] = g->agg_2hop_crs[_agg_first+i];
                    unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]];
                    unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]+1];
                    for (int j=first_0; j<last_0; ++j) {
                        assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_join(depth+1, 0, new_vec);
                        }
                    }
                }
                while (agg_first != agg_last) {
                    assignment[agg_order[depth]] = *agg_first;
                    if (depth == agg_plan.size() - 1) { ++result_size; }
                    else {
                        std::vector<int> new_vec;
                        recursive_agg_join(depth+1, 0, new_vec);
                    }
                    ++agg_first;
                }
            }
            else {
                std::vector<int> new_use_agg = use_agg;
                new_use_agg.push_back(hub_id);
                recursive_agg_join(depth, stage+1, new_use_agg);
            }
            // hubの場合は2段ジャンプ
            while (hub_first != hub_last) {
                assignment[general_detail[depth][stage]] = *hub_first;
                int len = general_plan[depth].size()-1;
                unsigned idx = general_plan[depth][len][0].first + assignment[general_plan[depth][len][0].second];
                unsigned *first = g->al_crs + g->al_keys[idx];
                unsigned *last = g->al_crs + g->al_keys[idx+1];
                match_nums[depth][0] = last - first;
                std::memcpy(intersect_store[depth][0], first, sizeof(unsigned)*(last-first));
                if (agg_plan[depth].size() == 1) {
                    while (first != last) {
                        assignment[general_order[depth]] = *first;
                        if (depth == general_plan.size() - 1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_join(depth+1, 0, new_vec);
                        }
                        ++first;
                    }
                }
                else { recursive_agg_join(depth, stage+1, use_agg); }
                ++hub_first;
            }
        }
        else {
            // intersection
            ++intersection_count;
            unsigned x_first = 0;
            unsigned x_last = match_nums[depth][stage-1];
            int match_num = 0;

            while (x_first != x_last && _agg_first != _agg_last) {
                if (intersect_store[depth][stage-1][x_first] < g->agg_2hop_crs[_agg_first]) { ++x_first; }
                else if (intersect_store[depth][stage-1][x_first] > g->agg_2hop_crs[_agg_first]) { ++_agg_first; }
                else {
                    intersect_store[depth][stage][match_num] = g->agg_2hop_crs[_agg_first];
                    ptr_store[depth][stage][hub_id][match_num] = _agg_first;
                    for (int j=0; j<use_agg.size(); ++j) {
                        ptr_store[depth][stage][use_agg[j]][match_num] = ptr_store[depth][stage-1][use_agg[j]][x_first];
                    }
                    ++match_num; ++x_first; ++_agg_first;
                }
            }
            match_nums[depth][stage] = match_num;
            std::vector<int> new_use_agg = use_agg;
            new_use_agg.push_back(hub_id);

            if (stage == agg_plan[depth].size()-1) {
                unsigned *intersect = intersect_store[depth][stage];
                int match_num = match_nums[depth][stage];
                for (int i=0; i<match_num; ++i) {
                    assignment[agg_order[depth]] = intersect[i];
                    if (new_use_agg.size() > 0) {
                        unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[0]][i]];
                        unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[0]][i]+1];
                        for (int j=first_0; j<last_0; ++j) {
                            assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                            if (new_use_agg.size() > 1) {
                                unsigned first_1 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[1]][i]];
                                unsigned last_1 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[1]][i]+1];
                                for (int k=first_1; k<last_1; ++k) {
                                    assignment[agg_detail[depth][1]] = g->agg_al_crs[k];
                                    if (depth == agg_order.size()-1) { ++result_size; }
                                    else {
                                        std::vector<int> new_vec;
                                        recursive_agg_join(depth+1, 0, new_vec);
                                    }
                                }
                            }
                            else {
                                if (depth == agg_order.size()-1) { ++result_size; }
                                else {
                                    std::vector<int> new_vec;
                                    recursive_agg_join(depth+1, 0, new_vec);
                                }
                            }
                        }
                    }
                    else {
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_join(depth+1, 0, new_vec);
                        }
                    }
                }
            }
            else { recursive_agg_join(depth, stage+1, new_use_agg); }

            // hub
            while (hub_first != hub_last) {
                ++intersection_count;
                assignment[general_detail[depth][hub_id]] = *hub_first;
                int len = general_plan[depth].size()-1;
                unsigned idx = general_plan[depth][len][stage].first + assignment[general_plan[depth][len][stage].second];
                unsigned *first = g->al_crs + g->al_keys[idx];
                unsigned *last = g->al_crs + g->al_keys[idx+1];
                x_first = 0;
                match_num = 0;
                
                while (x_first != x_last && first != last) {
                    if (intersect_store[depth][stage-1][x_first] < *first) { ++x_first; }
                    else if (intersect_store[depth][stage-1][x_first] > *first) { ++first; }
                    else {
                        intersect_store[depth][stage][match_num] = *first;
                        for (int j=0; j<use_agg.size(); ++j) {
                            ptr_store[depth][stage][use_agg[j]][match_num] = ptr_store[depth][stage-1][use_agg[j]][x_first];
                        }
                        ++match_num; ++x_first; ++first;
                    }
                }
                match_nums[depth][stage] = match_num;

                if (stage == agg_plan[depth].size()-1) {
                    unsigned *intersect = intersect_store[depth][stage];
                    int match_num = match_nums[depth][stage];
                    for (int i=0; i<match_num; ++i) {
                        assignment[general_order[depth]] = intersect[i];
                        if (use_agg.size() > 0) {
                            unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]];
                            unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]+1];
                            for (int j=first_0; j<last_0; ++j) {
                                assignment[general_detail[depth][0]] = g->agg_al_crs[j];
                                if (use_agg.size() > 1) {
                                    unsigned first_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]];
                                    unsigned last_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]+1];
                                    for (int k=first_1; k<last_1; ++k) {
                                        assignment[general_detail[depth][1]] = g->agg_al_crs[k];
                                        if (depth == general_order.size()-1) { ++result_size; }
                                        else {
                                            std::vector<int> new_vec;
                                            recursive_agg_join(depth+1, 0, new_vec);
                                        }
                                    }
                                }
                                else {
                                    if (depth == general_order.size()-1) { ++result_size; }
                                    else {
                                        std::vector<int> new_vec;
                                        recursive_agg_join(depth+1, 0, new_vec);
                                    }
                                }
                            }
                        }
                        else {
                            if (depth == general_order.size()-1) { ++result_size; }
                            else {
                                std::vector<int> new_vec;
                                recursive_agg_join(depth+1, 0, new_vec);
                            }
                        }
                    }
                }
                else { recursive_agg_join(depth, stage+1, use_agg); }
                ++hub_first;
            }
        }
    }
}


void AggExecutor::recursive_agg_cache_join(int depth, int stage, std::vector<int> &use_agg) {
    // キャッシュによりこのdepthの計算をしなくて良い場合
    if (stage == agg_plan[depth].size()) {
        // 集約隣接リストを使用していないと仮定したコードになっているため, 修正が必要な場合あり
        unsigned *intersect = intersect_store[depth][stage-1];
        int match_num = match_nums[depth][stage-1];
        for (int i=0; i<match_num; ++i) {
            assignment[agg_order[depth]] = intersect[i];
            for (int x=0; x<cache_reset[agg_order[depth]].size(); ++x) {
                auto [d, l] = cache_reset[agg_order[depth]][x];
                start_from[d] = std::min(start_from[d], l);
            }
            start_from[depth] = cache_set[depth];
            if (depth == agg_plan.size() - 1) { ++result_size; }
            else {
                std::vector<int> new_vec;
                recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
            }
        }
        return;
    }

    // use_agg: どのstageで集約隣接リストを使用したのか
    if (agg_plan[depth].size() - agg_detail[depth].size() > stage) {
        // normal mode, 普通に隣接リストをロードしていく
        unsigned idx = agg_plan[depth][stage].first + assignment[agg_plan[depth][stage].second];
        unsigned *first = g->al_crs + g->al_keys[idx];
        unsigned *last = g->al_crs + g->al_keys[idx+1];
        if (stage == 0) {
            match_nums[depth][0] = last - first;
            std::memcpy(intersect_store[depth][0], first, sizeof(unsigned)*(last-first));
            if (agg_plan[depth].size() == 1) {
                while (first != last) {
                    assignment[agg_order[depth]] = *first;
                    for (int x=0; x<cache_reset[agg_order[depth]].size(); ++x) {
                        auto [d, l] = cache_reset[agg_order[depth]][x];
                        start_from[d] = std::min(start_from[d], l);
                    }
                    start_from[depth] = cache_set[depth];
                    if (depth == agg_plan.size() - 1) { ++result_size; }
                    else {
                        std::vector<int> new_vec;
                        recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                    }
                    ++first;
                }
            }
            else { recursive_agg_cache_join(depth, stage+1, use_agg); }
        }
        else {
            // intersectionを行うモード
            ++intersection_count;
            unsigned x_first = 0;
            unsigned x_last = match_nums[depth][stage-1];

            int match_num = 0;
            while (x_first != x_last && first != last) {
                if (intersect_store[depth][stage-1][x_first] < *first) { ++x_first; }
                else if (intersect_store[depth][stage-1][x_first] > *first) { ++first; }
                else {
                    intersect_store[depth][stage][match_num] = *first;
                    for (int j=0; j<use_agg.size(); ++j) {
                        ptr_store[depth][stage][use_agg[j]][match_num] = ptr_store[depth][stage-1][use_agg[j]][x_first];
                    }
                    ++match_num; ++x_first; ++first;
                }
            }
            match_nums[depth][stage] = match_num;

            if (stage == agg_plan[depth].size()-1) {
                unsigned *intersect = intersect_store[depth][stage];
                int match_num = match_nums[depth][stage];
                for (int i=0; i<match_num; ++i) {
                    assignment[agg_order[depth]] = intersect[i];
                    for (int x=0; x<cache_reset[agg_order[depth]].size(); ++x) {
                        auto [d, l] = cache_reset[agg_order[depth]][x];
                        start_from[d] = std::min(start_from[d], l);
                    }
                    if (use_agg.size() > 0) {
                        unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]];
                        unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]+1];
                        for (int j=first_0; j<last_0; ++j) {
                            assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                            for (int x=0; x<cache_reset[agg_detail[depth][0]].size(); ++x) {
                                auto [d, l] = cache_reset[agg_detail[depth][0]][x];
                                start_from[d] = std::min(start_from[d], l);
                            }
                            if (use_agg.size() > 1) {
                                unsigned first_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]];
                                unsigned last_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]+1];
                                for (int k=first_1; k<last_1; ++k) {
                                    assignment[agg_detail[depth][1]] = g->agg_al_crs[k];
                                    for (int x=0; x<cache_reset[agg_detail[depth][1]].size(); ++x) {
                                        auto [d, l] = cache_reset[agg_detail[depth][1]][x];
                                        start_from[d] = std::min(start_from[d], l);
                                    }
                                    start_from[depth] = cache_set[depth];
                                    if (depth == agg_order.size()-1) { ++result_size; }
                                    else {
                                        std::vector<int> new_vec;
                                        recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                                    }
                                }
                            }
                            else {
                                start_from[depth] = cache_set[depth];
                                if (depth == agg_order.size()-1) { ++result_size; }
                                else {
                                    std::vector<int> new_vec;
                                    recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                                }
                            }
                        }
                    }
                    else {
                        start_from[depth] = cache_set[depth];
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                        }
                    }
                }
            }
            else { recursive_agg_cache_join(depth, stage+1, use_agg); }
        }
    }
    else {
        // agg_mode, 集約隣接リスト, ハブリストを使う
        int hub_id = stage+agg_detail[depth].size()-agg_plan[depth].size();
        unsigned agg_idx = agg_plan[depth][stage].first + assignment[agg_plan[depth][stage].second];
        unsigned hub_idx = general_plan[depth][hub_id][0].first + assignment[general_plan[depth][hub_id][0].second];
        unsigned _agg_first = g->agg_2hop_keys[agg_idx]; unsigned _agg_last = g->agg_2hop_keys[agg_idx+1];
        unsigned *agg_first = g->agg_2hop_crs + _agg_first; unsigned *agg_last = g->agg_2hop_crs + _agg_last;
        unsigned *hub_first = g->al_hub_crs + g->al_hub_keys[hub_idx]; unsigned *hub_last = g->al_hub_crs + g->al_hub_keys[hub_idx+1];

        if (stage == 0) {
            match_nums[depth][0] = agg_last - agg_first;
            std::memcpy(intersect_store[depth][0], agg_first, sizeof(unsigned)*(agg_last-agg_first));
            for (int i=0; i<_agg_last-_agg_first; ++i) {
                ptr_store[depth][0][0][i] = _agg_first+i;
            }
            if (agg_plan[depth].size() == 1) {
                for (int i=0; i<_agg_last-_agg_first; ++i) {
                    assignment[agg_order[depth]] = g->agg_2hop_crs[_agg_first+i];
                    for (int x=0; x<cache_reset[agg_order[depth]].size(); ++x) {
                        auto [d, l] = cache_reset[agg_order[depth]][x];
                        start_from[d] = std::min(start_from[d], l);
                    }
                    unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]];
                    unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]+1];
                    for (int j=first_0; j<last_0; ++j) {
                        assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                        for (int x=0; x<cache_reset[agg_detail[depth][0]].size(); ++x) {
                            auto [d, l] = cache_reset[agg_detail[depth][0]][x];
                            start_from[d] = std::min(start_from[d], l);
                        }
                        start_from[depth] = cache_set[depth];
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                        }
                    }
                }
                while (agg_first != agg_last) {
                    assignment[agg_order[depth]] = *agg_first;
                    for (int x=0; x<cache_reset[agg_order[depth]].size(); ++x) {
                        auto [d, l] = cache_reset[agg_order[depth]][x];
                        start_from[d] = std::min(start_from[d], l);
                    }
                    start_from[depth] = cache_set[depth];
                    if (depth == agg_plan.size() - 1) { ++result_size; }
                    else {
                        std::vector<int> new_vec;
                        recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                    }
                    ++agg_first;
                }
            }
            else {
                std::vector<int> new_use_agg = use_agg;
                new_use_agg.push_back(hub_id);
                recursive_agg_cache_join(depth, stage+1, new_use_agg);
            }
            // hubの場合は2段ジャンプ
            while (hub_first != hub_last) {
                assignment[general_detail[depth][stage]] = *hub_first;
                for (int x=0; x<cache_reset[general_detail[depth][stage]].size(); ++x) {
                    auto [d, l] = cache_reset[general_detail[depth][stage]][x];
                    start_from[d] = std::min(start_from[d], l);
                }
                int len = general_plan[depth].size()-1;
                unsigned idx = general_plan[depth][len][0].first + assignment[general_plan[depth][len][0].second];
                unsigned *first = g->al_crs + g->al_keys[idx];
                unsigned *last = g->al_crs + g->al_keys[idx+1];
                match_nums[depth][0] = last - first;
                std::memcpy(intersect_store[depth][0], first, sizeof(unsigned)*(last-first));
                if (agg_plan[depth].size() == 1) {
                    while (first != last) {
                        assignment[general_order[depth]] = *first;
                        for (int x=0; x<cache_reset[general_order[depth]].size(); ++x) {
                            auto [d, l] = cache_reset[general_order[depth]][x];
                            start_from[d] = std::min(start_from[d], l);
                        }
                        start_from[depth] = cache_set[depth];
                        if (depth == general_plan.size() - 1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                        }
                        ++first;
                    }
                }
                else { recursive_agg_cache_join(depth, stage+1, use_agg); }
                ++hub_first;
            }
        }
        else {
            // intersection
            ++intersection_count;
            unsigned x_first = 0;
            unsigned x_last = match_nums[depth][stage-1];
            int match_num = 0;

            while (x_first != x_last && _agg_first != _agg_last) {
                if (intersect_store[depth][stage-1][x_first] < g->agg_2hop_crs[_agg_first]) { ++x_first; }
                else if (intersect_store[depth][stage-1][x_first] > g->agg_2hop_crs[_agg_first]) { ++_agg_first; }
                else {
                    intersect_store[depth][stage][match_num] = g->agg_2hop_crs[_agg_first];
                    ptr_store[depth][stage][hub_id][match_num] = _agg_first;
                    for (int j=0; j<use_agg.size(); ++j) {
                        ptr_store[depth][stage][use_agg[j]][match_num] = ptr_store[depth][stage-1][use_agg[j]][x_first];
                    }
                    ++match_num; ++x_first; ++_agg_first;
                }
            }
            match_nums[depth][stage] = match_num;
            std::vector<int> new_use_agg = use_agg;
            new_use_agg.push_back(hub_id);

            if (stage == agg_plan[depth].size()-1) {
                unsigned *intersect = intersect_store[depth][stage];
                int match_num = match_nums[depth][stage];
                for (int i=0; i<match_num; ++i) {
                    assignment[agg_order[depth]] = intersect[i];
                    for (int x=0; x<cache_reset[agg_order[depth]].size(); ++x) {
                        auto [d, l] = cache_reset[agg_order[depth]][x];
                        start_from[d] = std::min(start_from[d], l);
                    }
                    if (new_use_agg.size() > 0) {
                        unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[0]][i]];
                        unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[0]][i]+1];
                        for (int j=first_0; j<last_0; ++j) {
                            assignment[agg_detail[depth][0]] = g->agg_al_crs[j];
                            for (int x=0; x<cache_reset[agg_detail[depth][0]].size(); ++x) {
                                auto [d, l] = cache_reset[agg_detail[depth][0]][x];
                                start_from[d] = std::min(start_from[d], l);
                            }
                            if (new_use_agg.size() > 1) {
                                unsigned first_1 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[1]][i]];
                                unsigned last_1 = g->agg_al_keys[ptr_store[depth][stage][new_use_agg[1]][i]+1];
                                for (int k=first_1; k<last_1; ++k) {
                                    assignment[agg_detail[depth][1]] = g->agg_al_crs[k];
                                    for (int x=0; x<cache_reset[agg_detail[depth][1]].size(); ++x) {
                                        auto [d, l] = cache_reset[agg_detail[depth][1]][x];
                                        start_from[d] = std::min(start_from[d], l);
                                    }
                                    start_from[depth] = cache_set[depth];
                                    if (depth == agg_order.size()-1) { ++result_size; }
                                    else {
                                        std::vector<int> new_vec;
                                        recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                                    }
                                }
                            }
                            else {
                                start_from[depth] = cache_set[depth];
                                if (depth == agg_order.size()-1) { ++result_size; }
                                else {
                                    std::vector<int> new_vec;
                                    recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                                }
                            }
                        }
                    }
                    else {
                        start_from[depth] = cache_set[depth];
                        if (depth == agg_order.size()-1) { ++result_size; }
                        else {
                            std::vector<int> new_vec;
                            recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                        }
                    }
                }
            }
            else { recursive_agg_cache_join(depth, stage+1, new_use_agg); }

            // hub
            while (hub_first != hub_last) {
                ++intersection_count;
                assignment[general_detail[depth][hub_id]] = *hub_first;
                for (int x=0; x<cache_reset[general_detail[depth][hub_id]].size(); ++x) {
                    auto [d, l] = cache_reset[general_detail[depth][hub_id]][x];
                    start_from[d] = std::min(start_from[d], l);
                }
                int len = general_plan[depth].size()-1;
                unsigned idx = general_plan[depth][len][stage].first + assignment[general_plan[depth][len][stage].second];
                unsigned *first = g->al_crs + g->al_keys[idx];
                unsigned *last = g->al_crs + g->al_keys[idx+1];
                x_first = 0;
                match_num = 0;
                
                while (x_first != x_last && first != last) {
                    if (intersect_store[depth][stage-1][x_first] < *first) { ++x_first; }
                    else if (intersect_store[depth][stage-1][x_first] > *first) { ++first; }
                    else {
                        intersect_store[depth][stage][match_num] = *first;
                        for (int j=0; j<use_agg.size(); ++j) {
                            ptr_store[depth][stage][use_agg[j]][match_num] = ptr_store[depth][stage-1][use_agg[j]][x_first];
                        }
                        ++match_num; ++x_first; ++first;
                    }
                }
                match_nums[depth][stage] = match_num;

                if (stage == agg_plan[depth].size()-1) {
                    unsigned *intersect = intersect_store[depth][stage];
                    int match_num = match_nums[depth][stage];
                    for (int i=0; i<match_num; ++i) {
                        assignment[general_order[depth]] = intersect[i];
                        for (int x=0; x<cache_reset[general_order[depth]].size(); ++x) {
                            auto [d, l] = cache_reset[general_order[depth]][x];
                            start_from[d] = std::min(start_from[d], l);
                        }
                        if (use_agg.size() > 0) {
                            unsigned first_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]];
                            unsigned last_0 = g->agg_al_keys[ptr_store[depth][stage][use_agg[0]][i]+1];
                            for (int j=first_0; j<last_0; ++j) {
                                assignment[general_detail[depth][0]] = g->agg_al_crs[j];
                                for (int x=0; x<cache_reset[general_detail[depth][0]].size(); ++x) {
                                    auto [d, l] = cache_reset[general_detail[depth][0]][x];
                                    start_from[d] = std::min(start_from[d], l);
                                }
                                if (use_agg.size() > 1) {
                                    unsigned first_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]];
                                    unsigned last_1 = g->agg_al_keys[ptr_store[depth][stage][use_agg[1]][i]+1];
                                    for (int k=first_1; k<last_1; ++k) {
                                        assignment[general_detail[depth][1]] = g->agg_al_crs[k];
                                        for (int x=0; x<cache_reset[general_detail[depth][1]].size(); ++x) {
                                            auto [d, l] = cache_reset[general_detail[depth][1]][x];
                                            start_from[d] = std::min(start_from[d], l);
                                        }
                                        start_from[depth] = cache_set[depth];
                                        if (depth == general_order.size()-1) { ++result_size; }
                                        else {
                                            std::vector<int> new_vec;
                                            recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                                        }
                                    }
                                }
                                else {
                                    start_from[depth] = cache_set[depth];
                                    if (depth == general_order.size()-1) { ++result_size; }
                                    else {
                                        std::vector<int> new_vec;
                                        recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                                    }
                                }
                            }
                        }
                        else {
                            start_from[depth] = cache_set[depth];
                            if (depth == general_order.size()-1) { ++result_size; }
                            else {
                                std::vector<int> new_vec;
                                recursive_agg_cache_join(depth+1, start_from[depth+1], new_vec);
                            }
                        }
                    }
                }
                else { recursive_agg_cache_join(depth, stage+1, use_agg); }
                ++hub_first;
            }
        }
    }
}
