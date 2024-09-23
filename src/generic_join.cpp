#include "../include/executor.hpp"


const unsigned BIT_MASK = BITSET_SIZE - 1;

void GenericJoin::decide_plan(LabeledGraph *lg, Query *query) {
    this->lg = lg;
    q = query;

    // 本来はここに実行計画の最適化が入る
    vertex_order = {0,1,2};
    // vertex_order = {2,0,1};
    // vertex_order = {0,2,1,3};
    // vertex_order = {2,3,1,0};
    // vertex_order = {0,1,3,2};
    // vertex_order = {1,3,0,2};
    // vertex_order = {1,3,2,0};
    // vertex_order = {1,2,3,0};
    // vertex_order = {1,2,0,3};
    // vertex_order = {0,1,2,3};
    // vertex_order = {0,1,2,3,4};
    // vertex_order = {3,4,2,1,0};
    // vertex_order = {2,0,1,4,5,3};
    // vertex_order = {2,3,1,4,5,0};
    // vertex_order = {5,6,4,3,2,1,0};
    setup();
}


void GenericJoin::setup() {
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

    intersection_result = new vector<unsigned>[num_variables-2];
    match_nums = new unsigned[num_variables-2];
    for (int i=0; i<descriptors.size(); ++i) {
        match_nums[i] = 0;
        intersection_result[i].resize(lg->num_v);
        size_t s = tmp_dsps[i].size();
        descriptors[i].resize(s);
        for (int j=0; j<s; ++j) { descriptors[i][j] = tmp_dsps[i][j]; }
    }
    tmp_result = new unsigned[lg->num_v];

    // cache setup
    cache_switch.clear();
    cache_available.clear();
    for (int i=0; i<descriptors.size(); ++i) {
        int max_src = -1;
        for (int j=0; j<descriptors[i].size(); ++j) {
            int src = get<2>(descriptors[i][j]);
            if (max_src < src) { max_src = src; }
        }
        if (max_src < i+1) {
            if (!cache_switch.contains(max_src)) {
                cache_switch.insert(unordered_map<int,vector<int>>::value_type (max_src, {}));
                cache_switch[max_src].reserve(descriptors[i].size());
            }
            cache_switch[max_src].push_back(i+2);
            cache_available[i+2] = false;
        }
    }

    result_size = 0;
    intersection_count = 0;
    keys = new unsigned[num_variables];

    for (int i=0; i<num_variables; i++) {
        update_num[i] = 0;
        empty_num[i] = 0;
        empty_runtime[i] = 0;
        lev_runtime[i] = 0;
    }
    bit_num_stat.resize(BITSET_SIZE+1);
    for (int i=0; i<=BITSET_SIZE; ++i) {
        bit_num_stat[i] = 0;
    }
}


void GenericJoin::run(string &method_name) {
    Chrono_t start = get_time();
    if (vertex_order.size() == 1) {
        get_single_assignment();
    }
    else {
        if (debug) { cout << "--------------------- Multiway Join ---------------------" << endl; }
        method_id = method_dict[method_name];
        multiway_join();
    }
    Chrono_t end = get_time();
    stat["runtime"] = to_string(get_msec_runtime(&start, &end));
    summarize();
}


void GenericJoin::get_single_assignment() {
    result_size = lg->num_v;
}


void GenericJoin::multiway_join() {
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
                    cache_available[cache_switch[0][j]] = false;
                }
            }
        }

        // ++intersection_count;
        recursive_join(2);

        if (cache_switch.contains(1)) {
            for (int j=0; j<cache_switch[1].size(); ++j) {
                cache_available[cache_switch[1][j]] = false;
            }
        }
    }
}


void GenericJoin::recursive_join(int current_depth) {
    switch (method_id) {
        case 0:
            find_assignables(current_depth);
            break;
        case 1:
            find_assignables_v2(current_depth);
            break;
        case 2:
            find_assignables_with_bitset(current_depth);
            break;
        case 3:
            find_assignables_with_dfilter(current_depth);
            break;
        default:
            find_assignables(current_depth);
            break;
    }
    int vir_depth = current_depth-2;

    if (current_depth == num_variables-1) {
        for (int i=0; i<match_nums[vir_depth]; ++i) {
            ++result_size;
        }
    }
    else {
        for (int i=0; i<match_nums[vir_depth]; ++i) {
            keys[current_depth] = intersection_result[vir_depth][i];
            recursive_join(current_depth+1);
            if (cache_switch.contains(current_depth)) {
                for (int j=0; j<cache_switch[current_depth].size(); ++j) {
                    cache_available[cache_switch[current_depth][j]] = false;
                }
            }
        }
    }
}


void GenericJoin::find_assignables(int current_depth) {
    int vir_depth = current_depth-2;
    if (cache_available.contains(current_depth) && cache_available[current_depth]) {
        return;
    }

    int arr_num = descriptors[vir_depth].size();
    match_nums[vir_depth] = 0;

    if (arr_num == 1) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        intersection_result[vir_depth].assign(first,last);
        match_nums[vir_depth] = last - first;
        if (cache_available.contains(current_depth)) { cache_available[current_depth] = true; }
        return;
    }
    
    // Chrono_t start = get_time();
    for (int i=0; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        if (first == last) {
            if (cache_available.contains(current_depth)) {
                cache_available[current_depth] = true;
            }
            return;
        }
        indices[i] = {first, last};
        // al_len_total += last-first;
    }

    ++intersection_count;
    //2. Sort indexes by their first value and do some initial iterators book-keeping!
    sort(indices, indices+arr_num,
        [&](const pair<unsigned*,unsigned*> &a,
        const pair<unsigned*,unsigned*> &b) { return *a.first < *b.first; });

    int it = 0;
    int match_num = 0;
    pair<unsigned*,unsigned*> *its = &indices[it];
    unsigned max = *indices[arr_num-1].first;
    bool b = false;

    // ++update_num[current_depth];
    while (true) {
        if (*its->first == max) {
            intersection_result[vir_depth][match_num] = max;
            ++match_num;
            if (++its->first == its->second) { break; }
        }
        while (*its->first < max) {
            if (++its->first == its->second) {
                b = true;
                break;
            }
        }
        if (b) { break; }
        max = *its->first;
        // if文でitがarr_num-1を超えないようにする
        if (++it > arr_num-1) { it = 0; }
        
        its = &indices[it];
    }
    match_nums[vir_depth] = match_num;
    if (cache_available.contains(current_depth)) { cache_available[current_depth] = true; }

    // Chrono_t end = get_time();
    // lev_runtime[current_depth] += get_msec_runtime(&start, &end);
    if (match_num == 0) { ++empty_num[current_depth]; }
    // match_num_total += match_num*2;
}


void GenericJoin::find_assignables_v2(int current_depth) {
    int vir_depth = current_depth-2;
    if (cache_available.contains(current_depth) && cache_available[current_depth]) {
        return;
    }
    int arr_num = descriptors[vir_depth].size();
    match_nums[vir_depth] = 0;

    if (arr_num == 1) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        intersection_result[vir_depth].assign(first,last);
        match_nums[vir_depth] = last - first;
        return;
    }
    for (int i=0; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        if (first == last) {
            if (cache_available.contains(current_depth)) {
                cache_available[current_depth] = true;
            }
            return;
        }
        indices[i] = {first, last};
    }

    //2. Sort indexes by their first value and do some initial iterators book-keeping!
    sort(indices, indices+arr_num,
        [&](const pair<unsigned*,unsigned*> &a,
        const pair<unsigned*,unsigned*> &b) { return *a.first < *b.first; });

    int it = 0;
    int match_num;
    pair<unsigned*,unsigned*> *it1 = &indices[it];
    pair<unsigned*,unsigned*> *it2;

    for (it=1; it<arr_num; ++it) {
        it2 = &indices[it];
        bool last_loop = it==arr_num-1;
        match_num = 0;
        while (it1->first != it1->second && it2->first != it2->second) {
            if (*it1->first < *it2->first) {
                ++it1->first;
                while (it1->first != it1->second && *it1->first < *it2->first) {
                    ++it1->first;
                }
            } else if (*it1->first > *it2->first) {
                ++it2->first;
                while (it2->first != it2->second && *it1->first > *it2->first) {
                    ++it2->first;
                }
            } else {
                if (last_loop) { intersection_result[vir_depth][match_num] = *it1->first; }
                else {
                    tmp_result[match_num] = *it1->first;
                }
                ++match_num;
                ++it1->first;
                ++it2->first;
            }
        }
        if (last_loop || !match_num) { break; }
        it1->first = tmp_result;
        it1->second = tmp_result + match_num;
    }
    // end = get_time();
    // lev_runtime[current_depth] += get_msec_runtime(&start, &end);
    if (match_num == 0) {
    //     empty_runtime[current_depth] += get_msec_runtime(&start, &end);
        ++empty_num[current_depth];
    }
    match_nums[vir_depth] = match_num;
    if (cache_available.contains(current_depth)) { cache_available[current_depth] = true; }
}


void GenericJoin::find_assignables_with_bitset(int current_depth) {
    int vir_depth = current_depth-2;
    if (cache_available.contains(current_depth) && cache_available[current_depth]) {
        return;
    }
    int arr_num = descriptors[vir_depth].size();
    match_nums[vir_depth] = 0;

    if (arr_num == 1) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        intersection_result[vir_depth].assign(first,last);
        match_nums[vir_depth] = last - first;
        return;
    }

    // Chrono_t start = get_time();
    int max_onebits = 0;
    for (int i=0; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        if (i) { filter_bs &= lg->adj_bs[idx]; }
        else { filter_bs = lg->adj_bs[idx]; }
        if (lg->adj_bs[idx].count() > max_onebits) { max_onebits = lg->adj_bs[idx].count(); }
        if (filter_bs.none()) {
            if (cache_available.contains(current_depth)) {
                cache_available[current_depth] = true;
            }
            return;
        }
        bit_num_stat[max_onebits] += 1;

        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];

        if (first == last) {
            if (cache_available.contains(current_depth)) {
                cache_available[current_depth] = true;
            }
            return;
        }
        indices[i] = {first, last};
    }
    ++intersection_count;

    //2. Sort indexes by their first value and do some initial iterators book-keeping!
    sort(indices, indices+arr_num,
        [&](const pair<unsigned*,unsigned*> &a,
        const pair<unsigned*,unsigned*> &b) { return *a.first < *b.first; });

    int it = 0;
    int match_num = 0;
    pair<unsigned*,unsigned*> *its = &indices[it];
    unsigned max = *indices[arr_num-1].first;
    bool b = false;

    // ++update_num[current_depth];
    while (true) {
        if (*its->first == max) {
            intersection_result[vir_depth][match_num] = max;
            ++match_num;
            if (++its->first == its->second) { break; }
        }
        
        while (!filter_bs[*its->first & (BIT_MASK)] || *its->first < max) {
            if (++its->first == its->second) {
                b = true;
                break;
            }
        }
        if (b) { break; }
        max = *its->first;
        // if文でitがarr_num-1を超えないようにする
        if (++it > arr_num-1) { it = 0; }
        
        its = &indices[it];
    }
    match_nums[vir_depth] = match_num;
    if (cache_available.contains(current_depth)) { cache_available[current_depth] = true; }

    // Chrono_t end = get_time();
    // lev_runtime[current_depth] += get_msec_runtime(&start, &end);

    if (match_num == 0) {
    //     empty_runtime[current_depth] += get_msec_runtime(&start, &end);
        ++empty_num[current_depth];
    }
}


void GenericJoin::find_assignables_with_dfilter(int current_depth) {
    int vir_depth = current_depth-2;
    if (cache_available.contains(current_depth) && cache_available[current_depth]) {
        return;
    }

    int arr_num = descriptors[vir_depth].size();
    match_nums[vir_depth] = 0;

    if (arr_num == 1) {
        auto [dir,el,src,dl] = descriptors[vir_depth][0];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        intersection_result[vir_depth].assign(first,last);
        match_nums[vir_depth] = last - first;
        if (cache_available.contains(current_depth)) { cache_available[current_depth] = true; }
        return;
    }
    
    // Chrono_t start = get_time();
    int min_dist = -1;
    int max_dist = -1;
    for (int i=0; i<arr_num; ++i) {
        auto [dir,el,src,dl] = descriptors[vir_depth][i];
        unsigned idx = lg->num_v * (lg->num_vl * (dir * lg->num_el + el) + dl) + keys[src];
        unsigned *first = lg->al_e_crs + lg->al_v_crs[idx];
        unsigned *last = lg->al_e_crs + lg->al_v_crs[idx+1];
        if (first == last) {
            if (cache_available.contains(current_depth)) {
                cache_available[current_depth] = true;
            }
            return;
        }
        indices[i] = {first, last};

        int dist = lg->dists[keys[src]];
        if (min_dist == -1) {
            min_dist = dist; max_dist = dist;
        }
        else {
            int dist_diff0 = dist - min_dist;
            int dist_diff1 = max_dist - dist;
            if (abs(dist_diff0)>2 || abs(dist_diff1)>2) {
                if (cache_available.contains(current_depth)) {
                    cache_available[current_depth] = true;
                }
                return;
            }
            else if (dist_diff0 < 0) { min_dist = dist; }
            else if (dist_diff1 < 0) { max_dist = dist; }
        }
    }

    ++intersection_count;
    //2. Sort indexes by their first value and do some initial iterators book-keeping!
    sort(indices, indices+arr_num,
        [&](const pair<unsigned*,unsigned*> &a,
        const pair<unsigned*,unsigned*> &b) { return *a.first < *b.first; });

    int it = 0;
    int match_num = 0;
    pair<unsigned*,unsigned*> *its = &indices[it];
    unsigned max = *indices[arr_num-1].first;
    bool b = false;

    // ++update_num[current_depth];
    while (true) {
        if (*its->first == max) {
            intersection_result[vir_depth][match_num] = max;
            ++match_num;
            if (++its->first == its->second) { break; }
        }
        while (*its->first < max) {
            if (++its->first == its->second) {
                b = true;
                break;
            }
        }
        if (b) { break; }
        max = *its->first;
        // if文でitがarr_num-1を超えないようにする
        if (++it > arr_num-1) { it = 0; }
        
        its = &indices[it];
    }
    match_nums[vir_depth] = match_num;
    if (cache_available.contains(current_depth)) { cache_available[current_depth] = true; }

    // Chrono_t end = get_time();
    // lev_runtime[current_depth] += get_msec_runtime(&start, &end);
    if (match_num == 0) { ++empty_num[current_depth]; }
}


void GenericJoin::summarize() {
    stat["dataset"] = lg->dataset_name;
    stat["query"] = q->query_name;
    stat["method"] = executor_name;
    stat["vertex_order"] = join(vertex_order, "-->");
    stat["result_size"] = to_string(result_size);

    cout << "--- Basic Info ---" << endl;
    cout << "Method: " << stat["method"] << "\t\tDataset: " << stat["dataset"] << "\t\tQuery: " << stat["query"] << endl;

    cout << "--- Runtimes ---" << endl;
    cout << "multiway join:\t\t\t" << stat["runtime"] << " [msec]" << endl;

    cout << "--- Results ---" << endl;
    cout << "result size:\t\t\t" << stat["result_size"] << endl;
    cout << "number of intermediate tuples:\t" << intersection_count << endl;

    cout << "--- Details ---" << endl;
    cout << "No interseciton counts of each depth:\t\t\t" << endl;
    print(empty_num);
    unsigned total = 0;
    for (auto &itr : empty_num) { total += itr.second; }
    cout << "Total: " << total << endl;
    stat["empty_num"] = to_string(total);
    // cout << "No need: " << to_string((double)(al_len_total - match_num_total)/al_len_total) << endl;

    // cout << "The interseciton counts of each depth:\t\t\t" << endl;
    // print(update_num);
    // total = 0;
    // for (auto &itr : update_num) { total += itr.second; }
    // cout << "Total: " << total << endl;

    // cout << "No interseciton of each depth:\t\t\t" << endl;
    // print(empty_runtime);
    // double total_d = 0;
    // for (auto &itr : empty_runtime) { total_d += itr.second; }
    // cout << "Total: " << total_d << endl;

    // cout << "The runtimes of each depth:\t\t\t" << endl;
    // print(lev_runtime);
    // double total_d = 0;
    // for (auto &itr : lev_runtime) { total_d += itr.second; }
    // cout << "Total: " << total_d << endl;

    stats->push_back(stat);

    ofstream fout("/Users/toshihiroito/implements/WebDB2024/output/special/bs_"+get_timestamp()+".txt");
    for (int i=0; i<=BITSET_SIZE; ++i) {
        fout << to_string(bit_num_stat[i]) << endl;
    }
}
