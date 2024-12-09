#include "include/util_new.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm> // clangだとsort()を使うには<algorithm>ヘッダを入れないとダメ

namespace fs = std::filesystem;


std::vector<std::vector<unsigned>> read_csv(std::ifstream &ifs_file, char delim) {
    std::string line_buf;
    std::vector<std::string> word_buf;
    std::vector<std::vector<unsigned>> table;
    while (getline(ifs_file, line_buf)) {
        std::istringstream i_stream(line_buf);
        word_buf = split(line_buf, delim);
        std::vector<unsigned> tmp_v(word_buf.size());
        for (int i=0; i<word_buf.size(); ++i) { tmp_v[i] = stoul(word_buf[i]); }
        table.push_back(tmp_v);
    }

    return table;
}


class Serializer {
public:
    std::unordered_map<unsigned,unsigned> vertices; // 頂点idと頂点ラベルの対応
    std::map<std::string, int> graph_info; // 頂点ラベルの種類数等を格納
    std::vector<std::vector<unsigned>> for_scan; // 頂点idを頂点ラベルごとに分割
    std::vector<std::vector<unsigned>> adjlist; // キー: num_v * (num_vl * (dir * num_el + el) + dl) + si
    std::vector<std::vector<unsigned>> hublist;
    std::vector<std::pair<unsigned, unsigned>> degree;
    std::map<unsigned, std::vector<unsigned>> *agg_al;

    Serializer() {}
    Serializer(std::string &input_dirpath, std::string &output_dirpath, std::unordered_set<std::string> &items);

    void read_vertices(std::string &vertex_filepath);
    void read_edges(std::string &edge_filepath);
    void construct_agg_al(bool filtered, unsigned long long size_limit);
    void dump_scan(std::string &output_dirpath);
    void dump_al(std::string &output_dirpath);
    void dump_agg_al(std::string &output_dirpath);
};


Serializer::Serializer(std::string &input_dirpath, std::string &output_dirpath, std::unordered_set<std::string> &items) {
    // 指定されたinput_dirpathが存在するかどうか
    if (!fs::exists(input_dirpath) || !fs::is_directory(input_dirpath)) {
        std::cerr << "PathError: input_dirpath '" << input_dirpath << "' is not exist." << std::endl;
        exit(1);
    }

    // 頂点データ, 辺データの読み込み
    std::string vertex_filepath = input_dirpath + "/vertices.csv";
    std::string edge_filepath = input_dirpath + "/edges.csv";
    read_vertices(vertex_filepath);
    read_edges(edge_filepath);

    // 出力先の場所が存在しない場合は新たにディレクトリを作成
    fs::create_directories(output_dirpath);

    // agg_al: 隣接リストをまとめたindex
    if (items.contains("agg_al")) {
        bool filtered = items.contains("filtered");
        // 環境変数からindexが使用可能な配列サイズを取得
        unsigned long long size_limit = std::stoull(getenv("SIZE_LIMIT"));
        // agg_alを実際に構築していく
        construct_agg_al(filtered, size_limit);
        // 書き込み
        dump_agg_al(output_dirpath);
    }
    dump_scan(output_dirpath);
    dump_al(output_dirpath);
}


void Serializer::read_vertices(std::string &vertex_filepath) {
    Chrono_t start = get_time();

    std::ifstream ifs_v_file(vertex_filepath);
    std::vector<std::vector<unsigned>> table = read_csv(ifs_v_file, ',');

    std::unordered_set<int> v_labels;
    for (int i=0; i<table.size(); ++i) {
        vertices[table[i][0]] = table[i][1];
        v_labels.insert(table[i][1]);
    }
    graph_info["num_v"] = vertices.size();
    graph_info["num_v_labels"] = v_labels.size();
    for_scan.resize(graph_info["num_v_labels"]);
    for (int i=0; i<table.size(); ++i) {
        for_scan[table[i][1]].push_back(table[i][0]);
    }

    Chrono_t end = get_time();
    double runtime = get_runtime(&start, &end);
    std::cout << "[Lap] Read vertices: " << std::to_string(runtime) << " [ms]" << std::endl;
}


void Serializer::read_edges(std::string &edge_filepath) {
    Chrono_t start = get_time();
    
    std::ifstream ifs_e_file(edge_filepath);
    std::vector<std::vector<unsigned>> table = read_csv(ifs_e_file, ',');
    std::vector<unsigned> empty_v;
    std::vector<std::pair<unsigned,unsigned>> empty_pair;
    std::unordered_set<int> e_labels;
    for (int i=0; i<table.size(); ++i) {
        e_labels.insert(table[i][2]);
    }
    graph_info["num_e"] = table.size();
    graph_info["num_e_labels"] = e_labels.size();

    // adjlist
    size_t adjlist_size = 2 * graph_info["num_e_labels"] * graph_info["num_v_labels"] * graph_info["num_v"];
    adjlist.resize(adjlist_size);
    hublist.resize(adjlist_size);

    for (int i=0; i<table.size(); ++i) {
        unsigned src_id = table[i][0];
        unsigned dst_id = table[i][1];
        unsigned edge_label = table[i][2];
        int src_label = vertices[src_id];
        int dst_label = vertices[dst_id];

        unsigned fwd_al_key = graph_info["num_v"] * (
            graph_info["num_v_labels"] * (0 * graph_info["num_e_labels"] + edge_label) + dst_label) + src_id;
        unsigned bwd_al_key = graph_info["num_v"] * (
            graph_info["num_v_labels"] * (1 * graph_info["num_e_labels"] + edge_label) + src_label) + dst_id;

        adjlist[fwd_al_key].push_back(dst_id);
        adjlist[bwd_al_key].push_back(src_id);
    }

    Chrono_t end = get_time();
    double runtime = get_runtime(&start, &end);
    std::cout << "[Lap] Read edges: " << std::to_string(runtime) << " [ms]" << std::endl;
}


void Serializer::construct_agg_al(bool filtered, unsigned long long size_limit) {
    Chrono_t start = get_time();

    // scans, agg_alを初期化
    int num_v = graph_info["num_v"];
    int num_v_labels = graph_info["num_v_labels"];
    int num_e_labels = graph_info["num_e_labels"];
    unsigned long long num_agg_al_keys = 4*num_v*num_v_labels*num_v_labels*num_e_labels*num_e_labels;
    agg_al = new std::map<unsigned, std::vector<unsigned>>[num_agg_al_keys];
    graph_info["agg_al_size"] = num_agg_al_keys;

    // グラフ上の全頂点を次数順でソート, scansも作成
    degree.resize(num_v);
    for (int v=0; v<num_v; ++v) {
        degree[v] = {v, 0};
        for (int i=0; i<2*num_v_labels*num_e_labels; ++i) {
            degree[v].second += adjlist[i*num_v+v].size();
        }
    }
    std::sort(degree.begin(), degree.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.second < b.second;});

    // indexの各種構成要素のサイズを見積もる
    unsigned long long num_2hops = 0;
    unsigned long long num_2paths = 0;
    unsigned long long num_hub_crs = 2 * graph_info["num_e"];
    unsigned long long num_al_keys = 2 * num_v * num_v_labels * num_e_labels;
    unsigned long long num_1paths = 2 * graph_info["num_e"];

    // サイズを見積りながら構築
    int idx = 0;
    while (num_agg_al_keys + (2*num_2hops) + num_2paths + num_v_labels + num_v + num_hub_crs + num_al_keys + num_1paths <= size_limit) {
        // 残っている中で最も次数が小さい頂点を取り出す
        auto [v, deg] = degree[idx];
        // vの隣接リストを参照してvを経由する全ての2-pathをindexに追加
        for (int i=0; i<2*num_v_labels*num_e_labels; ++i) {
            // dirの部分を逆方向にする計算
            int i_inv = i<num_v_labels*num_e_labels ? i+num_v_labels*num_e_labels : i-num_v_labels*num_e_labels;
            for (auto &u : adjlist[num_v*i+v]) {
                for (int j=0; j<2*num_v_labels*num_e_labels; ++j) {
                    unsigned key = num_v * (2 * num_v_labels * num_e_labels * i_inv + j) + u;
                    for (auto &w : adjlist[num_v*j+v]) {
                        // 2hop先の頂点が2hop元の頂点の隣接リストに含まれていなければフィルタリング
                        if (filtered && std::find(adjlist[num_v*i_inv+u].begin(), adjlist[num_v*i_inv+u].end(), w) == adjlist[num_v*i_inv+u].end()) {
                            continue;
                        }
                        if (!agg_al[key].contains(w)) {
                            agg_al[key].insert(std::map<unsigned, std::vector<unsigned>>::value_type (w, {}));
                            ++num_2hops;
                        }
                        agg_al[key][w].push_back(v);
                        ++num_2paths;
                    }
                }
            }
        }

        num_hub_crs -= deg;
        ++idx;
        if (idx >= num_v) { break; }
    }

    std::cout << "Size limit:\t" << std::to_string(size_limit) << std::endl;
    std::cout << "Index size:\t" << std::to_string(num_agg_al_keys + (2*num_2hops) + num_2paths + num_v_labels + num_v + num_hub_crs + num_al_keys + num_1paths) << std::endl;
    std::cout << "Breakdown:" << std::endl;
    std::cout << "\tNumAggregateAdjlistkeys: " << std::to_string(num_agg_al_keys) << std::endl;
    std::cout << "\tNum2hops: " << std::to_string(num_2hops) << std::endl;
    std::cout << "\tNum2paths: " << std::to_string(num_2paths) << std::endl;
    std::cout << "\tNumScansKeys: " << std::to_string(num_v_labels+1) << std::endl;
    std::cout << "\tNumScansSrcs: " << std::to_string(num_v) << std::endl;
    std::cout << "\tNumAdjlistKeys: " << std::to_string(num_al_keys) << std::endl;
    std::cout << "\tNum1paths: " << std::to_string(num_1paths) << std::endl;
    std::cout << "\tNumHubCrs: " << std::to_string(num_hub_crs) << std::endl;
    std::cout << "We eliminated " << std::to_string(num_v - idx) << " hubs." << std::endl;

    // indexに収まらなかった頂点をscansに挿入
    for (int i=idx; i<num_v; ++i) {
        unsigned v = degree[i].first;
        for (int dir=0; dir<2; ++dir) {
            for (int el=0; el<num_e_labels; ++el) {
                for (int dl=0; dl<num_v_labels; ++dl) {
                    for (auto &u : adjlist[num_v*(num_v_labels*(num_e_labels*dir+el)+dl)+v]) {
                        hublist[num_v*(num_v_labels*(num_e_labels*(!dir)+el)+vertices[v])+u].push_back(v);
                    }
                }
            }
        }
    }

    // hublist, agg_al の末端部分をソート
    for (int i=0; i<hublist.size(); ++i) {
        std::sort(hublist[i].begin(), hublist[i].end());
    }
    for (int i=0; i<num_agg_al_keys; ++i) {
        for (auto &item : agg_al[i]) {
            std::sort(item.second.begin(), item.second.end());
        }
    }

    Chrono_t end = get_time();
    double runtime = get_runtime(&start, &end);
    std::cout << "[Lap] Construct agg_al: " << std::to_string(runtime) << " [ms]" << std::endl;
}


void Serializer::dump_agg_al(std::string &output_dirpath) {
    Chrono_t start = get_time();

    std::ofstream fout1, fout2, fout3, fout4;
    fout1.open(output_dirpath + "/agg_2hop_key.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout2.open(output_dirpath + "/agg_2hop_crs.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout3.open(output_dirpath + "/agg_al_key.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout4.open(output_dirpath + "/agg_al_crs.bin", std::ios::out|std::ios::binary|std::ios::trunc);

    unsigned current_ptr0 = 0;
    unsigned current_ptr1 = 0;
    for (int i=0; i<graph_info["agg_al_size"]; ++i) {
        fout1.write((const char *) &current_ptr0, sizeof(unsigned));
        current_ptr0 += agg_al[i].size();
        for (auto &item : agg_al[i]) {
            fout2.write((const char *) &item.first, sizeof(unsigned));
            fout3.write((const char *) &current_ptr1, sizeof(unsigned));
            for (int k=0; k<item.second.size(); ++k) {
                fout4.write((const char *) &item.second[k], sizeof(unsigned));
            }
            current_ptr1 += item.second.size();
        }
    }
    fout1.write((const char *) &current_ptr0, sizeof(unsigned));
    fout3.write((const char *) &current_ptr1, sizeof(unsigned));

    fout1.close();
    fout2.close();
    fout3.close();
    fout4.close();

    Chrono_t end = get_time();

    double runtime = get_runtime(&start, &end);
    std::cout << "[Lap] Dump agg_al: " << std::to_string(runtime) << " [ms]" << std::endl;
}


void Serializer::dump_scan(std::string &output_dirpath) {
    std::ofstream fout1, fout2;
    // v_crsの要素数を決定
    unsigned scan_v_crs_len = graph_info["num_v_labels"];
    
    fout1.open(output_dirpath + "/scan_key.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout2.open(output_dirpath + "/scan_crs.bin", std::ios::out|std::ios::binary|std::ios::trunc);

    unsigned current_ptr = 0;
    for (int i=0; i<scan_v_crs_len; ++i) {
        fout1.write((const char*) &current_ptr, sizeof(unsigned));
        // 各パーティション内をソート
        std::sort(for_scan[i].begin(), for_scan[i].end());
        current_ptr += for_scan[i].size();
        for (int j=0; j<for_scan[i].size(); ++j) {
            fout2.write((const char *) &for_scan[i][j], sizeof(unsigned));
        }
    }

    fout1.write((const char *) &current_ptr, sizeof(unsigned));
    std::cout << "size of scan_v_crs: " << scan_v_crs_len+1 << std::endl;
    std::cout << "size of scan_e_crs: " << current_ptr << std::endl;
    fout1.close();
    fout2.close();
}


void Serializer::dump_al(std::string &output_dirpath) {
    Chrono_t start = get_time();

    std::ofstream fout1, fout2, fout3, fout4;
    fout1.open(output_dirpath + "/al_key.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout2.open(output_dirpath + "/al_crs.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout3.open(output_dirpath + "/al_hub_key.bin", std::ios::out|std::ios::binary|std::ios::trunc);
    fout4.open(output_dirpath + "/al_hub_crs.bin", std::ios::out|std::ios::binary|std::ios::trunc);

    unsigned current_ptr0 = 0;
    unsigned current_ptr1 = 0;
    unsigned al_v_crs_len = 2 * graph_info["num_e_labels"] * graph_info["num_v_labels"] * graph_info["num_v"];
    int max_degree = 0;
    for (int i=0; i<al_v_crs_len; ++i) {
        fout1.write((const char *) &current_ptr0, sizeof(unsigned));
        fout3.write((const char *) &current_ptr1, sizeof(unsigned));
        // 各パーティション内をソート
        sort(adjlist[i].begin(), adjlist[i].end());
        current_ptr0 += adjlist[i].size();
        current_ptr1 += hublist[i].size();
        max_degree = std::max(max_degree, (int)adjlist[i].size());
        for (int j=0; j<adjlist[i].size(); ++j) {
            fout2.write((const char *) &adjlist[i][j], sizeof(unsigned));
        }
        for (int j=0; j<hublist[i].size(); ++j) {
            fout4.write((const char *) &hublist[i][j], sizeof(unsigned));
        }
    }
    fout1.write((const char *) &current_ptr0, sizeof(unsigned));
    fout3.write((const char *) &current_ptr1, sizeof(unsigned));
    graph_info["max_degree"] = max_degree;

    fout1.close();
    fout2.close();
    fout3.close();
    fout4.close();

    fout1.open(output_dirpath + "/data_info.txt", std::ios::out|std::ios::trunc);
    for (auto &item : graph_info) {
        fout1 << item.first << "," << std::to_string(item.second) << std::endl;
        std::cout << item.first << ":\t" << std::to_string(item.second) << std::endl;
    }

    Chrono_t end = get_time();
    double runtime = get_runtime(&start, &end);
    std::cout << "[Lap] Dump al: " << std::to_string(runtime) << " [ms]" << std::endl;
}


int main(int argc, char* argv[]) {
    // serialize.cppの引数: ./exec.out --input_dirpath <input_dirpath> --output_dirpath <output_dirpath> --items <item> .. <item>
    // input_dirpath: グラフ作成元データへのパス
    // output_dirpath: シリアライズデータを保存するパス
    // items: 追加で作成する拡張データ (複数指定可)

    if (argc < 5) {
        std::cerr << "ArgumentError: Some arguments were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --input_dirpath <input_dirpath> --output_dirpath <output_dirpath> --items <item> .. <item>" << std::endl;
        return 1;
    }

    std::string input_dirpath;
    std::string output_dirpath;
    std::unordered_set<std::string> items;

    if (std::strcmp(argv[1], "--input_dirpath") == 0) { input_dirpath = argv[2]; }
    else {
        std::cerr << "ArgumentError: The argument 'input_dirpath' were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --input_dirpath <input_dirpath> --output_dirpath <output_dirpath> --items <item> .. <item>" << std::endl;
        return 1;
    }

    if (std::strcmp(argv[3], "--output_dirpath") == 0) { output_dirpath = argv[4]; }
    else {
        std::cerr << "ArgumentError: The argument 'output_dirpath' were missing.\n" <<
        "Usage: "<< argv[0] <<
        " --input_dirpath <input_dirpath> --output_dirpath <output_dirpath> --items <item> .. <item>" << std::endl;
        return 1;
    }

    if (argc > 5) {
        if (std::strcmp(argv[5], "--items") == 0) {
            for (int i=6; i<argc; ++i) { items.insert(argv[i]); }
        }
        else {
            std::cerr << "ArgumentError: Invalid argument: " << argv[5] << "\n" <<
            "Usage: "<< argv[0] <<
            " --input_dirpath <input_dirpath> --output_dirpath <output_dirpath> --items <item> .. <item>" << std::endl;
            return 1;
        }
    }

    Chrono_t start = get_time();
    Serializer(input_dirpath, output_dirpath, items);
    Chrono_t end = get_time();

    std::cout << std::endl << "Total runtime: " << std::to_string(get_runtime(&start, &end) / 1000.0) << " [sec]" << std::endl;

    return 0;
}