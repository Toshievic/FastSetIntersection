#include "../include/executor.hpp"

#include <numeric>
#include <cstring>


const unsigned BIT_MASK = BITSET_SIZE - 1;
constexpr unsigned long long base = 1;

void AlphaGenericJoin::setup() {
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
    // indicesの配列長は固定のため、考えうるintersection処理を行う最大のリスト数分の領域を確保
    indices = new pair<unsigned*,unsigned*>[max_size];

    result_store.resize(num_variables-2);
    validate_pool.resize(num_variables-2);
    match_nums.resize(num_variables-2);
    bs_store.resize(num_variables-2);
    calc_level = new int[num_variables];
    idxes = new unsigned[num_variables];
    available_level.clear();
    has_full_cache.clear();
    for (int i=0; i<descriptors.size(); ++i) {
        size_t s = tmp_dsps[i].size();
        descriptors[i].resize(s);
        result_store[i].resize(s);
        validate_pool[i].resize(s);
        match_nums[i].resize(s);
        calc_level[i] = -1;
        bs_store[i] = new bitset<BITSET_SIZE>[s];
        for (int j=0; j<s; ++j) {
            descriptors[i][j] = tmp_dsps[i][j];
            result_store[i][j] = new unsigned[lg->max_degree];
            match_nums[i][j] = 0;
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
    keys = new unsigned[num_variables];
    for (int i=0; i<3; ++i) { dist_counter[i] = 0; }

    for (int i=0; i<num_variables; i++) {
        update_num[i] = 0;
        empty_num[i] = 0;
        empty_runtime[i] = 0;
        lev_runtime[i] = 0;
    }
}


void AlphaGenericJoin::multiway_join() {
    // 0,1番目の頂点をscanによって得る
    unsigned idx = lg->num_vl * (lg->num_vl * (scanned_dir * lg->num_el + scanned_el) + scanned_sl) + scanned_dl;
    unsigned first = lg->scan_v_crs[idx];
    unsigned last = lg->scan_v_crs[idx+1];
    if (first == last) { return; }
    unsigned src_prev = lg->scan_src_crs[0];

    for (int i=first; i<last; i++) {
        keys[0] = lg->scan_src_crs[i];
        keys[1] = lg->scan_dst_crs[i];

        if (src_prev != keys[0]) {
            src_prev = keys[0];
            if (cache_switch.contains(0)) {
                for (int j=0; j<cache_switch[0].size(); ++j) {
                    available_level[cache_switch[0][j].first] = cache_switch[0][j].second;
                }
            }
        }
        recursive_join(2);
        if (cache_switch.contains(1)) {
            for (int j=0; j<cache_switch[1].size(); ++j) {
                auto [v, level] = cache_switch[1][j];
                available_level[v] = min(available_level[v], level);
            }
        }
    }
}


void AlphaGenericJoin::recursive_join(int current_depth) {
    switch (method_id) {
        case 0:
            find_assignables_v2(current_depth);
            break;
        case 1:
            find_assignables_exp(current_depth);
            break;
        case 2:
            find_assignables_with_bitset(current_depth);
            break;
        case 3:
            find_assignables_with_2hop(current_depth);
            break;
        default:
            break;
    }
    int vir_depth = current_depth-2;

    int s = match_nums[vir_depth].back();

    if (current_depth == num_variables-1) {
        for (int i=0; i<s; ++i) { ++result_size; }
    }
    else {
        unsigned *result_itr = result_store[vir_depth].back();
        for (int i=0; i<s; ++i) {
            keys[current_depth] = result_itr[i];
            recursive_join(current_depth+1);
            if (cache_switch.contains(current_depth)) {
                for (int j=0; j<cache_switch[current_depth].size(); ++j) {
                    auto [v,level] = cache_switch[current_depth][j];
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


void AlphaGenericJoin::find_assignables(int current_depth) {
    int vir_depth = current_depth-2;
    int level = available_level.contains(current_depth) ? available_level[current_depth] : -2;
    int arr_num = result_store[vir_depth].size();
    // 最終的なintersection結果を利用可能な場合はすぐreturn
    if (level == arr_num-1) { return; }
    for (int i=level==-2 ? 0 : level+1; i<arr_num; ++i) { match_nums[vir_depth][i] = 0; }

    if (level < 0) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        match_nums[vir_depth][0] = last - first;
        memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));
        if (arr_num == 1) {
            if (level == -2) { return; }
            if (has_full_cache.contains(current_depth)) { available_level[current_depth] = 0; }
            else { available_level[current_depth] = -1; }
            return;
        }
        ++level;
    }
    ++intersection_count;
    
    for (int i=level+1; i<arr_num; ++i) {
        pair<unsigned*, unsigned*> it1 = {result_store[vir_depth][i-1], result_store[vir_depth][i-1]+match_nums[vir_depth][i-1]};
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];

        pair<unsigned*, unsigned*> it2 = {lg->al_e_crs+lg->al_v_crs[idx], lg->al_e_crs+lg->al_v_crs[idx+1]};

        int match_num = 0;
        num_comparison = 0;
        // if (it1.first == it1.second || it2.first == it2.second) { ++num_comparison; }
        // Chrono_t start = get_time();
        while (it1.first != it1.second && it2.first != it2.second) {
            if (*it1.first < *it2.first) {
                ++it1.first;
                while (it1.first != it1.second && *it1.first < *it2.first) {
                    ++it1.first;
                }
            } else if (*it1.first > *it2.first) {
                ++it2.first;
                while (it2.first != it2.second && *it1.first > *it2.first) {
                    ++it2.first;
                }
            } else {
                result_store[vir_depth][i][match_num] = *it1.first;
                ++match_num;
                ++it1.first; ++it2.first;
            }
        }
        // Chrono_t end = get_time();
        // lev_runtime[current_depth] += get_msec_runtime(&start, &end);
        match_nums[vir_depth][i] = match_num;
        if (!match_num) { break; }
    }
    if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
    else { available_level[current_depth] = arr_num-2; }
}


void AlphaGenericJoin::find_assignables_v2(int current_depth) {
    int vir_depth = current_depth-2;
    int level = available_level.contains(current_depth) ? available_level[current_depth] : -2;
    int arr_num = result_store[vir_depth].size();
    // 最終的なintersection結果を利用可能な場合はすぐreturn
    if (level == arr_num-1) { return; }
    for (int i=level==-2 ? 0 : level+1; i<arr_num; ++i) { match_nums[vir_depth][i] = 0; }

    if (level < 0) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        match_nums[vir_depth][0] = last - first;
        memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));
        if (arr_num == 1) {
            if (level == -2) { return; }
            if (has_full_cache.contains(current_depth)) { available_level[current_depth] = 0; }
            else { available_level[current_depth] = -1; }
            return;
        }
        ++level;
    }
    
    for (int i=level+1; i<arr_num; ++i) {
        pair<unsigned*, unsigned*> it1 = {result_store[vir_depth][i-1], result_store[vir_depth][i-1]+match_nums[vir_depth][i-1]};
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];

        pair<unsigned*, unsigned*> it2 = {lg->al_e_crs+lg->al_v_crs[idx], lg->al_e_crs+lg->al_v_crs[idx+1]};

        int match_num = 0;
        ++intersection_count;
        while (it1.first != it1.second && it2.first != it2.second) {
            if (*it1.first < *it2.first) { ++it1.first;}
            else if (*it1.first > *it2.first) { ++it2.first;}
            else {
                result_store[vir_depth][i][match_num] = *it1.first;
                ++match_num;
                ++it1.first; ++it2.first;
            }
        }
        match_nums[vir_depth][i] = match_num;
        if (!match_num) { break; }
    }
    if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
    else { available_level[current_depth] = arr_num-2; }
}


// 全てのintersection処理の前にBitFilterを完全に生成する方法
void AlphaGenericJoin::find_assignables_with_bitset(int current_depth) {
    int vir_depth = current_depth-2;
    int level = available_level.contains(current_depth) ? available_level[current_depth] : -2;
    int arr_num = result_store[vir_depth].size();
    // 最終的なintersection結果を利用可能な場合はすぐreturn
    if (level == arr_num-1) { return; }
    for (int i=level==-2 ? 0 : level+1; i<arr_num; ++i) { match_nums[vir_depth][i] = 0; }

    if (level < 0) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        match_nums[vir_depth][0] = last - first;
        memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));
        if (arr_num == 1) {
            if (level == -2) { return; }
            if (has_full_cache.contains(current_depth)) { available_level[current_depth] = 0; }
            else { available_level[current_depth] = -1; }
            return;
        }
        ++level;
        bs_store[vir_depth][0] = lg->adj_bs[idx];
    }

    for (int i=level+1; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        bs_store[vir_depth][i] = bs_store[vir_depth][i-1] & lg->adj_bs[idx];
        if (bs_store[vir_depth][i].none()) {
            if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
            else { available_level[current_depth] = arr_num-2; }
            match_nums[vir_depth][i] = 0;
            calc_level[i] = current_depth;
            return;
        }
        idxes[i] = idx;
    }

    ++intersection_count;
    for (int i=level+1; i<arr_num; ++i) {
        pair<unsigned*, unsigned*> it1 = {result_store[vir_depth][i-1], result_store[vir_depth][i-1]+match_nums[vir_depth][i-1]};
        unsigned first = lg->al_v_crs[idxes[i]];
        unsigned last = lg->al_v_crs[idxes[i]+1];
        pair<unsigned*, unsigned*> it2 = {lg->al_e_crs+first, lg->al_e_crs+last};
        int match_num = 0;

        while (it1.first != it1.second && it2.first != it2.second) {
            if (*it1.first < *it2.first) {
                ++it1.first;
                while (it1.first != it1.second && *it1.first < *it2.first) {
                    ++it1.first;
                }
            } else if (*it1.first > *it2.first) {
                ++it2.first;
                while (it2.first != it2.second && *it1.first > *it2.first) {
                    ++it2.first;
                }
            } else {
                result_store[vir_depth][i][match_num] = *it1.first;
                ++match_num;
                ++it1.first;
                ++it2.first;
            }
        }
        match_nums[vir_depth][i] = match_num;
        if (!match_num) { break; }
    }

    if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
    else { available_level[current_depth] = arr_num-2; }
}


void AlphaGenericJoin::find_assignables_with_2hop(int current_depth) {
    int vir_depth = current_depth-2;
    int level = available_level.contains(current_depth) ? available_level[current_depth] : -2;
    int arr_num = result_store[vir_depth].size();
    // 最終的なintersection結果を利用可能な場合はすぐreturn
    if (level == arr_num-1) { return; }
    for (int i=level==-2 ? 0 : level+1; i<arr_num; ++i) { match_nums[vir_depth][i] = 0; }

    if (level < 0) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        match_nums[vir_depth][0] = last - first;
        memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));
        if (arr_num == 1) {
            if (level == -2) { return; }
            if (has_full_cache.contains(current_depth)) { available_level[current_depth] = 0; }
            else { available_level[current_depth] = -1; }
            return;
        }
        ++level;
        validate_pool[vir_depth][level] = (unsigned long long)keys[src]*lg->p1 + (dir<<1)*lg->num_v;
    }
    
    for (int i=level+1; i<arr_num; ++i) {
        pair<unsigned*, unsigned*> it1 = {result_store[vir_depth][i-1], result_store[vir_depth][i-1]+match_nums[vir_depth][i-1]};
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        // 2-hop indexによる枝刈り
        unsigned long long base_h = keys[src] + (dir^1) * lg->num_v;
        for (int j=0; j<i; ++j) {
            unsigned long long h = (validate_pool[vir_depth][j] + base_h) & (lg->bs-1);
            if ((lg->twohop_bs[h>>6] & (base<<(h&63))) == 0) {
                if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
                else { available_level[current_depth] = arr_num-2; }
                match_nums[vir_depth][i] = 0;
                return;
            }
        }

        pair<unsigned*, unsigned*> it2 = {lg->al_e_crs+lg->al_v_crs[idx], lg->al_e_crs+lg->al_v_crs[idx+1]};

        int match_num = 0;

        while (it1.first != it1.second && it2.first != it2.second) {
            if (*it1.first < *it2.first) { ++it1.first; }
            else if (*it1.first > *it2.first) { ++it2.first; }
            else {
                result_store[vir_depth][i][match_num] = *it1.first;
                ++match_num;
                ++it1.first; ++it2.first;
            }
        }
        match_nums[vir_depth][i] = match_num;
        if (!match_num) { break; }

        validate_pool[vir_depth][i] = keys[src] * lg->p1 + (dir<<1) * lg->num_v;
    }
    ++intersection_count;
    if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
    else { available_level[current_depth] = arr_num-2; }
}


void AlphaGenericJoin::find_assignables_exp(int current_depth) {
    int vir_depth = current_depth-2;
    int level = available_level.contains(current_depth) ? available_level[current_depth] : -2;
    int arr_num = result_store[vir_depth].size();
    // 最終的なintersection結果を利用可能な場合はすぐreturn
    if (level == arr_num-1) { return; }
    for (int i=level==-2 ? 0 : level+1; i<arr_num; ++i) { match_nums[vir_depth][i] = 0; }

    if (level < 0) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        match_nums[vir_depth][0] = last - first;
        memcpy(result_store[vir_depth][0], first, sizeof(unsigned)*(last-first));
        if (arr_num == 1) {
            if (level == -2) { return; }
            if (has_full_cache.contains(current_depth)) { available_level[current_depth] = 0; }
            else { available_level[current_depth] = -1; }
            return;
        }
        ++level;
    }
    int min_dist = -1;
    int max_dist = -1;
    for (int i=0; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        int dist = lg->dists[keys[src]];
        if (min_dist == -1) { min_dist = dist; max_dist = dist; }
        else {
            if (dist < min_dist) { min_dist = dist; }
            else if (dist > max_dist) { max_dist = dist; }
        }
    }
    dist_counter[max_dist-min_dist] += 1;

    ++intersection_count;
    for (int i=level+1; i<arr_num; ++i) {
        pair<unsigned*, unsigned*> it1 = {result_store[vir_depth][i-1], result_store[vir_depth][i-1]+match_nums[vir_depth][i-1]};
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];

        pair<unsigned*, unsigned*> it2 = {lg->al_e_crs+lg->al_v_crs[idx], lg->al_e_crs+lg->al_v_crs[idx+1]};

        int match_num = 0;

        while (it1.first != it1.second && it2.first != it2.second) {
            if (*it1.first < *it2.first) {
                ++it1.first;
                while (it1.first != it1.second && *it1.first < *it2.first) {
                    ++it1.first;
                }
            } else if (*it1.first > *it2.first) {
                ++it2.first;
                while (it2.first != it2.second && *it1.first > *it2.first) {
                    ++it2.first;
                }
            } else {
                result_store[vir_depth][i][match_num] = *it1.first;
                ++match_num;
                ++it1.first;
                ++it2.first;
            }
        }
        match_nums[vir_depth][i] = match_num;
        if (!match_num) { break; }
    }
    if (has_full_cache.contains(current_depth)) { available_level[current_depth]=arr_num-1; }
    else { available_level[current_depth] = arr_num-2; }
}
