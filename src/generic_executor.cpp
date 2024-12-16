#include "../include/executor_new.hpp"

#include <cstring>


void GenericExecutor::init() {
    // 各種変数の初期化
    assignment = new unsigned[order.size()];
    plan.resize(order.size()-1);
    std::vector<std::vector<std::pair<unsigned,int>>> tmp_plan(order.size()-1);

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
        unsigned key = g->graph_info["num_v"] * (g->graph_info["num_v_labels"] * (is_bwd * g->graph_info["num_e_labels"] + el) + dl);
        tmp_plan[order_inv[dst]-1].push_back({key, src});
    }

    result_store.resize(order.size()-1);
    match_nums.resize(order.size()-1);
    
    for (int i=0; i<tmp_plan.size(); ++i) {
        plan[i].resize(tmp_plan[i].size());
        result_store[i].resize(plan[i].size());
        match_nums[i].resize(plan[i].size());
        for (int j=0; j<tmp_plan[i].size(); ++j) {
            plan[i][j] = tmp_plan[i][j];
            result_store[i][j] = new unsigned[g->graph_info["max_degree"]];
            match_nums[i][j] = 0;
        }
    }

    // cache setup
    for (int i=0; i<plan.size(); ++i) {
        for (int j=0; j<plan[i].size(); ++j) {
            int src = plan[i][j].second;
            if (order_inv[src] < i+1) {
                available_level[order[i+1]] = -1;
                if (!cache_switch.contains(src)) {
                    cache_switch.insert(std::unordered_map<int,std::vector<std::pair<int,int>>>::value_type (src,{}));
                    cache_switch.reserve(plan.size());
                }
                cache_switch[src].push_back({order[i+1],j-1});
                if (j == plan[i].size()-1) { has_full_cache.insert(order[i+1]); }
            }
        }
    }

    result_size = 0;
    intersection_count = 0;
}


void GenericExecutor::run(std::unordered_map<std::string, std::string> &options) {
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


void GenericExecutor::join() {
    unsigned first = g->scan_keys[scan_vl];
    unsigned last = g->scan_keys[scan_vl+1];

    for (int i=first; i<last; ++i) {
        assignment[order[0]] = i;
        recursive_join(1);
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void GenericExecutor::cache_join() {
    unsigned first = g->scan_keys[scan_vl];
    unsigned last = g->scan_keys[scan_vl+1];

    for (int i=first; i<last; ++i) {
        assignment[order[0]] = i;
        if (cache_switch.contains(0)) {
            for (int j=0; j<cache_switch[0].size(); ++j) {
                available_level[cache_switch[0][j].first] = cache_switch[0][j].second;
            }
        }
        recursive_cache_join(1);
    }
    exec_stats["intersection_count"] = intersection_count;
    exec_stats["result_size"] = result_size;
}


void GenericExecutor::recursive_join(int depth) {
    int vir_depth = depth - 1;
    int num_intersects = plan[vir_depth].size();
    
    unsigned idx = plan[vir_depth][0].first + assignment[plan[vir_depth][0].second];
    unsigned *first = g->al_crs + g->al_keys[idx];
    unsigned *last = g->al_crs + g->al_keys[idx+1];
    match_nums[vir_depth][0] = last - first;
    std::memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));

    for (int i=1; i<num_intersects; ++i) {
        ++intersection_count;
        unsigned *x_first = result_store[vir_depth][i-1];
        unsigned *x_last = result_store[vir_depth][i-1] + match_nums[vir_depth][i-1];
        unsigned y_idx = plan[vir_depth][i].first + assignment[plan[vir_depth][i].second];
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
                result_store[vir_depth][i][match_num] = *x_first;
                ++match_num;
                ++x_first;
                ++y_first;
            }
        }
        match_nums[vir_depth][i] = match_num;
        if (match_num == 0) { break; }
    }

    unsigned *result = result_store[vir_depth][num_intersects-1];
    int match_num = match_nums[vir_depth][num_intersects-1];
    for (int i=0; i<match_num; ++i) {
        assignment[order[depth]] = result[i];
        if (depth == order.size()-1) {
            ++result_size;
        }
        else {
            recursive_join(depth+1);
        }
    }
}


void GenericExecutor::recursive_cache_join(int depth) {
    find_assignables(depth);
    int vir_depth = depth - 1;
    int num_intersects = plan[vir_depth].size();

    unsigned *result = result_store[vir_depth][num_intersects-1];
    int match_num = match_nums[vir_depth][num_intersects-1];
    for (int i=0; i<match_num; ++i) {
        assignment[order[depth]] = result[i];
        if (depth == order.size()-1) {
            ++result_size;
        }
        else {
            recursive_cache_join(depth+1);
            if (cache_switch.contains(depth)) {
                for (int j=0; j<cache_switch[depth].size(); ++j) {
                    auto [v,level] = cache_switch[depth][j];
                    available_level[v] = std::min(available_level[v], level);
                }
            }
        }
    }
}


void GenericExecutor::find_assignables(int depth) {
    int vir_depth = depth - 1;
    int num_intersects = plan[vir_depth].size();

    int level = available_level.contains(order[depth]) ? available_level[order[depth]] : -2;
    if (level == num_intersects-1) { return; }
    
    if (level < 0) {
        unsigned idx = plan[vir_depth][0].first + assignment[plan[vir_depth][0].second];
        unsigned *first = g->al_crs + g->al_keys[idx];
        unsigned *last = g->al_crs + g->al_keys[idx+1];
        match_nums[vir_depth][0] = last - first;
        std::memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));
        if (level == -2) { return; }
        ++level;
    }

    for (int i=level+1; i<num_intersects; ++i) {
        ++intersection_count;
        unsigned *x_first = result_store[vir_depth][i-1];
        unsigned *x_last = result_store[vir_depth][i-1] + match_nums[vir_depth][i-1];
        unsigned y_idx = plan[vir_depth][i].first + assignment[plan[vir_depth][i].second];
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
                result_store[vir_depth][i][match_num] = *x_first;
                ++match_num;
                ++x_first;
                ++y_first;
            }
        }
        match_nums[vir_depth][i] = match_num;
        if (match_num == 0) { break; }
    }

    if (has_full_cache.contains(order[depth])) { available_level[order[depth]] = num_intersects-1; }
    else { available_level[order[depth]] = num_intersects-2; }
}


void GenericExecutor::summarize_result(std::unordered_map<std::string, std::string> &options) {
    using namespace std;
    cout << "--- Basic Info ---" << endl;
    cout << "Method: " << executor_name << "\t\tQuery: " << q->query_name << endl;
    cout << "Options:" << endl;
    for (auto &item : options) {
        cout << item.first << ":\t" << item.second << endl;
    }
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
