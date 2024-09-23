#include "../include/labeled_graph.hpp"
#include "../include/util.hpp"

#include <numeric>
#include <unordered_set>
#include <set>
#include <cstring>


const unsigned BUFFER_SIZE = 1024;

void LabeledGraph::load() {
    string workdir_path = getenv("WORKDIR_PATH");
    string info_file = workdir_path + "data/data_info.txt";
    string scan_v_file = workdir_path + "data/scan_v_crs.bin";
    string scan_src_file = workdir_path + "data/scan_src_crs.bin";
    string scan_dst_file = workdir_path + "data/scan_dst_crs.bin";
    string al_v_file = workdir_path + "data/al_v_crs.bin";
    string al_e_file = workdir_path + "data/al_e_crs.bin";

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

    unsigned av_size = static_cast<size_t>(fin_av.tellg()) / sizeof(unsigned);
    unsigned ae_size = static_cast<size_t>(fin_ae.tellg()) / sizeof(unsigned);
    al_v_crs = new unsigned[av_size];
    al_e_crs = new unsigned[ae_size];

    fin_av.seekg(0);
    fin_ae.seekg(0);

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

    fin_av.close();
    fin_ae.close();

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

    create_twohop_index();
}


void LabeledGraph::create_twohop_index() {
    twohop_v_crs = new unsigned[num_v+1];
    vector<vector<unsigned>> twohop_index(num_v);
    twohop_v_crs[0] = 0;
    unsigned long size_total = 0;
    for (int i=0; i<num_v; ++i) {
        // 隣接する頂点を列挙
        unordered_set<unsigned> neighbors;
        for (int j=0; j<2*num_vl*num_el; ++j) {
            unsigned start = al_v_crs[num_v*j+i];
            unsigned end = al_v_crs[num_v*j+i+1];
            for (unsigned p=start; p<end; ++p) { neighbors.insert(al_e_crs[p]); }
        }
        // 2-hop setの列挙
        set<unsigned> two_hops;
        for (auto &n: neighbors) {
            for (int j=0; j<2*num_vl*num_el; ++j) {
                unsigned start = al_v_crs[num_v*j+n];
                unsigned end = al_v_crs[num_v*j+n+1];
                for (unsigned p=start; p<end; ++p) {
                    if (al_e_crs[p] != i) { two_hops.insert(al_e_crs[p]); }
                }
            }
        }
        // twohop_indexへ入れる
        twohop_index[i].assign(two_hops.begin(), two_hops.end());
        sort(twohop_index[i].begin(), twohop_index[i].end());
        size_total += twohop_index[i].size();
        twohop_v_crs[i+1] = size_total;
    }

    twohop_e_crs = new unsigned[size_total];
    unsigned long current_ptr = 0;
    for (int i=0; i<num_v; ++i) {
        memcpy(twohop_e_crs+current_ptr, twohop_index[i].data(), twohop_index[i].size()*sizeof(unsigned));
        current_ptr += twohop_index[i].size();
    }

    // 2-hop indexのサイズを確認してみる
    cout << "Total size: " << size_total << endl << "num_v: " << num_v << endl << "fill rate: " << (float) size_total / (num_v*num_v) << endl;
}
