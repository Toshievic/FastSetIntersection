#include "../include/labeled_graph.hpp"
#include "../include/util.hpp"

#include <numeric>
#include <unordered_set>
#include <cstring>
#include <queue>
#include <cstdlib>


const unsigned BUFFER_SIZE = 1024;

void LabeledGraph::load() {
    string input_dir_path = find_path("", "input");
    string info_file = input_dir_path + "data_info.txt";
    string scan_v_file = input_dir_path + "scan_v_crs.bin";
    string scan_src_file = input_dir_path + "scan_src_crs.bin";
    string scan_dst_file = input_dir_path + "scan_dst_crs.bin";
    string al_v_file = input_dir_path + "al_v_crs.bin";
    string al_e_file = input_dir_path + "al_e_crs.bin";
    string twohop_file = input_dir_path + "2hop_bs.bin";

    // infoの読み込み
    ifstream fin_info(info_file, ios::in);
    if (!fin_info) {
        cout << "Cannot open file: " << info_file << endl;
        exit(1);
    }
    string line;
    getline(fin_info, line);
    num_v = stoul(line);
    getline(fin_info, line);
    num_vl = stoul(line);
    getline(fin_info, line);
    num_el = stoul(line);
    getline(fin_info, line);
    p1 = stoull(line);
    getline(fin_info, line);
    p2 = stoull(line);
    getline(fin_info, line);
    bs = stoull(line);
    fin_info.close();

    // edge scan専用データの読み込み
    // ifstream::ate -> ストリームの最後にシーク
    ifstream fin_sv(scan_v_file, ios::in|ifstream::ate|ios::binary);
    if (!fin_sv) {
        cout << "Cannot open file: " << scan_v_file << endl;
        exit(1);
    }
    ifstream fin_ss(scan_src_file, ios::in|ifstream::ate|ios::binary);
    if (!fin_ss) {
        cout << "Cannot open file: " << scan_src_file << endl;
        exit(1);
    }
    ifstream fin_sd(scan_dst_file, ios::in|ios::binary);
    if (!fin_sd) {
        cout << "Cannot open file: " << scan_dst_file << endl;
        exit(1);
    }

    unsigned sv_size = static_cast<size_t>(fin_sv.tellg()) / sizeof(unsigned);
    unsigned se_size = static_cast<size_t>(fin_ss.tellg()) / sizeof(unsigned);
    scan_v_crs = new unsigned[sv_size];
    scan_src_crs = new unsigned[se_size];
    scan_dst_crs = new unsigned[se_size];

    // ストリーム位置を最初に戻す
    fin_sv.seekg(0);
    fin_ss.seekg(0);

    unsigned current_ptr = 0;
    bool at_end = false;
    while (!at_end) {
        unsigned num_block = BUFFER_SIZE;
        if (current_ptr + BUFFER_SIZE > sv_size) {
            num_block = sv_size - current_ptr;
            at_end = true;
        }
        fin_sv.read(reinterpret_cast<char*>(scan_v_crs+current_ptr), num_block*sizeof(unsigned));
        current_ptr += BUFFER_SIZE;
    }

    current_ptr = 0;
    at_end = false;
    while (!at_end) {
        unsigned num_block = BUFFER_SIZE;
        if (current_ptr + BUFFER_SIZE > se_size) {
            num_block = se_size - current_ptr;
            at_end = true;
        }
        fin_ss.read(reinterpret_cast<char*>(scan_src_crs+current_ptr), num_block*sizeof(unsigned));
        fin_sd.read(reinterpret_cast<char*>(scan_dst_crs+current_ptr), num_block*sizeof(unsigned));
        current_ptr += BUFFER_SIZE;
    }

    fin_sv.close();
    fin_ss.close();
    fin_sd.close();

    // 隣接リストの読み込み
    ifstream fin_av(al_v_file, ios::in|ifstream::ate|ios::binary);
    if (!fin_av) {
        cout << "Cannot open file: " << al_v_file << endl;
        exit(1);
    }
    ifstream fin_ae(al_e_file, ios::in|ifstream::ate|ios::binary);
    if (!fin_ae) {
        cout << "Cannot open file: " << al_e_file << endl;
        exit(1);
    }
    ifstream fin_2h(twohop_file, ios::in|ifstream::ate|ios::binary);
    if (!fin_2h) {
        cout << "Cannot open file: " << twohop_file << endl;
        exit(1);
    }

    unsigned av_size = static_cast<size_t>(fin_av.tellg()) / sizeof(unsigned);
    unsigned ae_size = static_cast<size_t>(fin_ae.tellg()) / sizeof(unsigned);
    unsigned long long th_size = static_cast<size_t>(fin_2h.tellg()) / sizeof(unsigned long long);
    al_v_crs = new unsigned[av_size];
    al_e_crs = new unsigned[ae_size];
    twohop_bs.resize(th_size);

    fin_av.seekg(0);
    fin_ae.seekg(0);
    fin_2h.seekg(0);

    current_ptr = 0;
    at_end = false;
    while (!at_end) {
        unsigned num_block = BUFFER_SIZE;
        if (current_ptr + BUFFER_SIZE > av_size) {
            num_block = av_size - current_ptr;
            at_end = true;
        }
        fin_av.read(reinterpret_cast<char*>(al_v_crs+current_ptr), num_block*sizeof(unsigned));
        current_ptr += BUFFER_SIZE;
    }
    unsigned al_v_crs_size = current_ptr;

    current_ptr = 0;
    at_end = false;
    while (!at_end) {
        unsigned num_block = BUFFER_SIZE;
        if (current_ptr + BUFFER_SIZE > ae_size) {
            num_block = ae_size - current_ptr;
            at_end = true;
        }
        fin_ae.read(reinterpret_cast<char*>(al_e_crs+current_ptr), num_block*sizeof(unsigned));
        current_ptr += BUFFER_SIZE;
    }

    current_ptr = 0;
    at_end = false;
    vector<unsigned long long> tmp(BUFFER_SIZE);
    while (!at_end) {
        unsigned num_block = BUFFER_SIZE;
        if (current_ptr + BUFFER_SIZE > th_size) {
            num_block = th_size - current_ptr;
            at_end = true;
        }
        fin_2h.read(reinterpret_cast<char*>(tmp.data()), num_block*sizeof(unsigned long long));
        memcpy(twohop_bs.data()+current_ptr, tmp.data(), num_block*sizeof(unsigned long long));
        current_ptr += BUFFER_SIZE;
    }

    fin_av.close();
    fin_ae.close();
    fin_2h.close();

    // adj_bs,max_degreeを作成
    adj_bs = new bitset<BITSET_SIZE>[al_v_crs_size];
    max_degree = 0;

    for (int i=1; i<al_v_crs_size; ++i) {
        unsigned first = al_v_crs[i-1];
        unsigned last = al_v_crs[i];
        if (max_degree < last-first) { max_degree = last-first; }
        adj_bs[i-1].reset();
        for (int j=first; j<last; ++j) {
            // k=隣接頂点 mod BITSET_SIZEとして, k番目のbitを1にする
            unsigned h = al_e_crs[j] & (BITSET_SIZE-1);
                adj_bs[i-1].set(h);
        }
    }

    set_dists(0);
}


void LabeledGraph::set_dists(unsigned root_id) {
    dists = new unsigned[num_v];
    for (int i=0; i<num_v; ++i) {
        dists[i] = -1; // -1は初期値
    }
    queue<unsigned> q;

    dists[root_id] = 0;
    q.push(root_id);
    int counter = 0;

    while (!q.empty()) {
        unsigned v = q.front();
        q.pop();

        for (int i=0; i<2*num_vl*num_el; ++i) {
            unsigned start = al_v_crs[num_v*i+v];
            unsigned end = al_v_crs[num_v*i+v+1];
            for (unsigned p=start; p<end; ++p) {
                if (dists[al_e_crs[p]] == -1) { // 未訪問であれば
                    dists[al_e_crs[p]] = dists[v] + 1;
                    q.push(al_e_crs[p]);
                    if (dists[v] == 0) { ++counter; }
                }
            }
        }
    }
}
