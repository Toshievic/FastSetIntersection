#include "../include/master.hpp"
#include "../include/util.hpp"

#include <unordered_set>


int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <dataset_name>" << endl;
        exit(1);
    }
    string data_dir = dataset_master.at(argv[1]);

    // 頂点データの読み込み
    unordered_map<unsigned,unsigned> vertices; // 頂点idと頂点ラベルの対応
    unordered_set<int> num_v_labels; // 頂点ラベルの種類数

    string vertex_file = data_dir + "vertex_label.csv";
    ifstream ifs_v_file(vertex_file);
    vector<vector<unsigned>> table = read_csv(ifs_v_file, ',');

    for (int i=0; i<table.size(); ++i) {
        vertices[table[i][0]] = table[i][1];
        num_v_labels.insert(table[i][1]);
    }

    // エッジデータの読み込み
    // キーの決定方法: num_vl * (num_vl * (dir * num_el + el) + sl) + dl
    unordered_map<unsigned, vector<pair<unsigned,unsigned>>> scanned_edges;
    // キーの決定方法: num_v * (num_vl * (dir * num_el + el) + dl) + si
    unordered_map<unsigned, vector<unsigned>> adjlist;
    unordered_set<int> num_e_labels;

    string edge_file = data_dir + "edge_label.csv";
    ifstream ifs_e_file(edge_file);
    table = read_csv(ifs_e_file, ',');
    vector<unsigned> empty_v;
    vector<pair<unsigned,unsigned>> empty_pair;

    for (int i=0; i<table.size(); ++i) {
        unsigned src_id = table[i][0];
        unsigned dst_id = table[i][1];
        unsigned edge_label = table[i][2];
        int src_label = vertices[src_id];
        int dst_label = vertices[dst_id];
        num_e_labels.insert(edge_label);

        unsigned fwd_scan_key = num_v_labels.size() * (num_v_labels.size() * (0 * num_e_labels.size() + edge_label) + src_label) + dst_label;
        unsigned bwd_scan_key = num_v_labels.size() * (num_v_labels.size() * (1 * num_e_labels.size() + edge_label) + dst_label) + src_label;
        unsigned fwd_al_key = vertices.size() * (num_v_labels.size() * (0 * num_e_labels.size() + edge_label) + dst_label) + src_id;
        unsigned bwd_al_key = vertices.size() * (num_v_labels.size() * (1 * num_e_labels.size() + edge_label) + src_label) + dst_id;

        if (!scanned_edges.contains(fwd_scan_key)) { scanned_edges[fwd_scan_key] = empty_pair; }
        if (!scanned_edges.contains(bwd_scan_key)) { scanned_edges[bwd_scan_key] = empty_pair; }
        if (!adjlist.contains(fwd_al_key)) { adjlist[fwd_al_key] = empty_v; }
        if (!adjlist.contains(bwd_al_key)) { adjlist[bwd_al_key] = empty_v; }

        scanned_edges[fwd_scan_key].push_back({src_id,dst_id});
        scanned_edges[bwd_scan_key].push_back({dst_id,src_id});
        adjlist[fwd_al_key].push_back(dst_id);
        adjlist[bwd_al_key].push_back(src_id);
    }

    // v_crsの要素数を決定
    unsigned scan_v_crs_len = 2 * num_e_labels.size() * num_v_labels.size() * num_v_labels.size();
    unsigned al_v_crs_len = 2 * num_e_labels.size() * num_v_labels.size() * vertices.size();

    ofstream fout1, fout2, fout3;
    fout1.open("/Users/toshihiroito/implements/WebDB2024/data/scan_v_crs.bin", ios::out|ios::binary|ios::trunc);
    if (!fout1) {
        cout << "Cannot open file." << endl;
        exit(1);
    }
    fout2.open("/Users/toshihiroito/implements/WebDB2024/data/scan_src_crs.bin", ios::out|ios::binary|ios::trunc);
    if (!fout2) {
        cout << "Cannot open file." << endl;
        exit(1);
    }
    fout3.open("/Users/toshihiroito/implements/WebDB2024/data/scan_dst_crs.bin", ios::out|ios::binary|ios::trunc);
    if (!fout3) {
        cout << "Cannot open file." << endl;
        exit(1);
    }

    unsigned current_ptr = 0;
    for (int i=0; i<scan_v_crs_len; ++i) {
        fout1.write((const char*) &current_ptr, sizeof(unsigned));
        if (!scanned_edges.contains(i)) { continue; }
        // 各パーティション内をソート
        sort(scanned_edges[i].begin(), scanned_edges[i].end());
        current_ptr += scanned_edges[i].size();
        for (int j=0; j<scanned_edges[i].size(); ++j) {
            fout2.write((const char *) &scanned_edges[i][j].first, sizeof(unsigned));
            fout3.write((const char *) &scanned_edges[i][j].second, sizeof(unsigned));
        }
    }
    fout1.write((const char *) &current_ptr, sizeof(unsigned));
    cout << "size of scan_v_crs: " << scan_v_crs_len+1 << endl;
    cout << "size of scan_e_crs: " << current_ptr << endl;
    fout1.close();
    fout2.close();
    fout3.close();

    fout1.open("/Users/toshihiroito/implements/WebDB2024/data/al_v_crs.bin", ios::out|ios::binary|ios::trunc);
    if (!fout1) {
        cout << "Cannot open file." << endl;
        exit(1);
    }
    fout2.open("/Users/toshihiroito/implements/WebDB2024/data/al_e_crs.bin", ios::out|ios::binary|ios::trunc);
    if (!fout2) {
        cout << "Cannot open file." << endl;
        exit(1);
    }

    current_ptr = 0;
    for (int i=0; i<al_v_crs_len; ++i) {
        fout1.write((const char *) &current_ptr, sizeof(unsigned));
        if (!adjlist.contains(i)) { continue; }
        // 各パーティション内をソート
        sort(adjlist[i].begin(), adjlist[i].end());
        current_ptr += adjlist[i].size();
        for (int j=0; j<adjlist[i].size(); ++j) {
            fout2.write((const char *) &adjlist[i][j], sizeof(unsigned));
        }
    }
    fout1.write((const char *) &current_ptr, sizeof(unsigned));

    cout << "size of al_v_crs: " << al_v_crs_len+1 << endl;
    cout << "size of al_e_crs: " << current_ptr << endl;
    fout1.close();
    fout2.close();


    fout1.open("/Users/toshihiroito/implements/WebDB2024/data/data_info.txt", ios::out|ios::trunc);
    if (!fout1) {
        cout << "Cannot open file." << endl;
        exit(1);
    }
    fout1 << vertices.size() << endl
    << num_v_labels.size() << endl
    << num_e_labels.size() << endl;

    return 0;
}
