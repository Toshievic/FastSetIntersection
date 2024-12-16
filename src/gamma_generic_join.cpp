#include "../include/executor.hpp"

#include <cstring>
#include <algorithm>

constexpr unsigned long long base = 1;


void GammaGenericJoin::setup() {
    num_variables = vertex_order.size();
    v_index.clear();
    for (int i=0; i<num_variables; i++) { v_index[vertex_order[i]] = i; }

    descriptors.resize(num_variables-2);
    vector<vector<tuple<int,int,int,int>>> tmp_dsps(num_variables-2);
    unsigned max_size = 0;

    // 1,2番めの頂点は連結されてること
    for (auto &triple : q->triples) {
        int src = v_index[triple[0]];
        int el = triple[1];
        int dst = v_index[triple[2]];
        int sl = q->variables[triple[0]];
        int dl = q->variables[triple[2]];
        int is_bwd = false;

        if (src > dst) {
            int tmp = src;
            src = dst; dst = tmp;
            tmp = sl;
            sl = dl; dl = tmp;
            is_bwd = true;
        }
        if (src == 0 && dst == 1) {
            scanned_sl = sl; scanned_dl = dl;
            scanned_el = el; scanned_dir = is_bwd;
            continue;
        }
        tmp_dsps[dst-2].push_back({is_bwd,el,src,dl});
        if (max_size < tmp_dsps[dst-2].size()) { max_size = tmp_dsps[dst-2].size(); }
    }

    result_store.resize(num_variables-2);
    validate_pool.resize(num_variables-2);
    dist_base.resize(num_variables-2);
    match_nums.resize(num_variables-2);
    bs_store.resize(num_variables-2);
    calc_level = new int[num_variables];
    idxes = new unsigned[num_variables];
    deg = (int) lg->max_degree;
    available_level.clear();
    has_full_cache.clear();
    for (int i=0; i<descriptors.size(); ++i) {
        size_t s = tmp_dsps[i].size();
        descriptors[i].resize(s);
        result_store[i].resize(s);
        validate_pool[i].resize(s);
        dist_base[i].resize(s);
        match_nums[i].resize(s);
        calc_level[i] = -1;
        bs_store[i] = new bitset<BITSET_SIZE>[s];
        for (int j=0; j<s; ++j) {
            descriptors[i][j] = tmp_dsps[i][j];
            result_store[i][j] = new unsigned[3*deg];
            match_nums[i][j] = new unsigned[3];
            for (int k=0; k<3; ++k) { match_nums[i][j][k] = 0; }
        }
        // srcが小さい順でdescriptors[i]をソート
        sort(descriptors[i].begin(), descriptors[i].end(),
            [&] (tuple<int,int,int,int> &a, tuple<int,int,int,int> &b) { return get<2>(a) < get<2>(b); });
    }

    // cache setup
    for (int i=0; i<descriptors.size(); ++i) {
        for (int j=0; j<descriptors[i].size(); ++j) {
            int src = get<2>(descriptors[i][j]);
            if (src < i+1) {
                available_level[i+2] = -1;
                if (!cache_switch.contains(src)) {
                    cache_switch.insert(unordered_map<int,vector<pair<int,int>>>::value_type (src,{}));
                    cache_switch.reserve(descriptors.size());
                }
                cache_switch[src].push_back({i+2,j-1});
                if (j == descriptors[i].size()-1) { has_full_cache.insert(i+2); }
            }
        }
    }

    result_size = 0;
    intersection_count = 0;
    num_comparison = 0;
    keys = new unsigned[num_variables];
    for (int i=0; i<3; ++i) { dist_counter[i] = 0; }

    for (int i=0; i<num_variables; i++) {
        update_num[i] = 0;
        empty_num[i] = 0;
        empty_runtime[i] = 0;
        lev_runtime[i] = 0;
    }
}


void GammaGenericJoin::recursive_join(int current_depth) {
    find_assignables(current_depth);
    int vir_depth = current_depth-2;

    unsigned *s = match_nums[vir_depth].back();

    if (current_depth == num_variables-1) {
        for (int i=0; i<s[0]; ++i) { ++result_size; }
        for (int i=0; i<s[1]; ++i) { ++result_size; }
        for (int i=0; i<s[2]; ++i) { ++result_size; }
    }
    else {
        unsigned *result_itr = result_store[vir_depth].back();
        for (int j=0; j<s[0]; ++j) {
            keys[current_depth] = result_itr[j];
            recursive_join(current_depth+1);
            if (cache_switch.contains(current_depth)) {
                for (int k=0; k<cache_switch[current_depth].size(); ++k) {
                    auto [v,level] = cache_switch[current_depth][k];
                    available_level[v] = min(available_level[v], level);
                }
            }
            if (calc_level[current_depth] > -1) {
                available_level[calc_level[current_depth]] = 0;
                calc_level[current_depth] = -1;
            }
        }
        for (int j=0; j<s[1]; ++j) {
            keys[current_depth] = result_itr[j+deg];
            recursive_join(current_depth+1);
            if (cache_switch.contains(current_depth)) {
                for (int k=0; k<cache_switch[current_depth].size(); ++k) {
                    auto [v,level] = cache_switch[current_depth][k];
                    available_level[v] = min(available_level[v], level);
                }
            }
            if (calc_level[current_depth] > -1) {
                available_level[calc_level[current_depth]] = 0;
                calc_level[current_depth] = -1;
            }
        }
        for (int j=0; j<s[2]; ++j) {
            keys[current_depth] = result_itr[j+deg*2];
            recursive_join(current_depth+1);
            if (cache_switch.contains(current_depth)) {
                for (int k=0; k<cache_switch[current_depth].size(); ++k) {
                    auto [v,level] = cache_switch[current_depth][k];
                    available_level[v] = min(available_level[v], level);
                }
            }
            if (calc_level[current_depth] > -1) {
                available_level[calc_level[current_depth]] = 0;
                calc_level[current_depth] = -1;
            }
        }
    }
}


void GammaGenericJoin::find_assignables(int current_depth) {
    int vir_depth = current_depth-2;
    int level = available_level.contains(current_depth) ? available_level[current_depth] : -2;
    int arr_num = result_store[vir_depth].size();
    // 最終的なintersection結果を利用可能な場合はすぐreturn
    if (level == arr_num-1) { return; }
    for (int i=level==-2 ? 0 : level+1; i<arr_num; ++i) {
        match_nums[vir_depth][i][0] = 0;
        match_nums[vir_depth][i][1] = 0;
        match_nums[vir_depth][i][2] = 0;
    }

    if (level < 0) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned base_idx = 3 * (lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src]);
        unsigned *base = lg->al_crs + lg->al_keys[base_idx];
        unsigned *first = lg->al_crs + lg->al_keys[base_idx+1];
        unsigned *second = lg->al_crs + lg->al_keys[base_idx+2];
        unsigned *third = lg->al_crs + lg->al_keys[base_idx+3];

        match_nums[vir_depth][0][0] = first - base;
        match_nums[vir_depth][0][1] = second - first;
        match_nums[vir_depth][0][2] = third - second;

        memcpy(result_store[vir_depth][0], base, sizeof(unsigned)*(first-base));
        memcpy(result_store[vir_depth][0]+deg, first, sizeof(unsigned)*(second-first));
        memcpy(result_store[vir_depth][0]+2*deg, second, sizeof(unsigned)*(third-second));
    
        if (arr_num == 1) {
            if (level == -2) { return; }
            if (has_full_cache.contains(current_depth)) { available_level[current_depth] = 0; }
            else { available_level[current_depth] = -1; }
            return;
        }
        ++level;
        dist_base[vir_depth][0] = lg->dists[keys[src]] - 1;
    }
    int min_dist = dist_base[vir_depth][0]; int max_dist = min_dist + 2;

    for (int i=level+1; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        // 距離ラベルの比較
        int dist = lg->dists[keys[src]];
        int dist_diff0 = dist - min_dist; int dist_diff1 = max_dist - dist;
        // srcの距離ラベルが元のラベルより1以上大きい
        if (dist_diff0 > 1) { min_dist = dist-1; }
        // srcの距離ラベルが元のラベルより1以上小さい
        else if (dist_diff1 > 1) { max_dist = dist+1; }
        // max_dist - min_distが負の値であれば解なし
        if (max_dist < min_dist) { break; }
        
        unsigned base_idx = 3 * (lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src]);
        int lev1 = min_dist - dist_base[vir_depth][i-1];
        int lev2 = dist_diff0 < 1 ? dist_diff0*-1 + 1 : 0;
        int group_num = max_dist - min_dist + 1;
        bool flg = false;

        for (int j=0; j<group_num; ++j) {
            int it1_len = match_nums[vir_depth][i-1][j+lev1];
            if (it1_len > 0) {
                pair<unsigned*, unsigned*> it2 = {
                    lg->al_crs+lg->al_keys[base_idx+j+lev2],
                    lg->al_crs+lg->al_keys[base_idx+j+lev2+1]};
                if (it2.first != it2.second) {
                    pair<unsigned*, unsigned*> it1 = {
                        result_store[vir_depth][i-1]+(j+lev1)*deg, result_store[vir_depth][i-1]+(j+lev1)*deg+it1_len};

                    unsigned match_num = 0;
                    ++intersection_count;
                    while (it1.first != it1.second && it2.first != it2.second) {
                        if (*it1.first < *it2.first) { ++it1.first;}
                        else if (*it1.first > *it2.first) { ++it2.first;}
                        else {
                            result_store[vir_depth][i][j*deg+match_num] = *it1.first;
                            ++match_num;
                            ++it1.first; ++it2.first;
                        }
                    }
                    match_nums[vir_depth][i][j] = match_num;
                    if (match_num) { flg = true; }
                }
            }
        }
        dist_base[vir_depth][i] = min_dist;
        if (!flg) { break; }
    }

    if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
    else { available_level[current_depth] = arr_num-2; }
}
