#ifndef LABELED_GRAPH_HPP
#define LABELED_GRAPH_HPP

#include "master.hpp"

#include <vector>
#include <bitset>

#define BITSET_SIZE 4096


class LabeledGraph {
private:
    bool debug;
public:
    string dataset_name;
    unsigned num_v, num_vl, num_el, max_degree;
    // fwd or bwd, (src_label)-[edge_label]-(dst_label)を満たすエッジを全scanする用
    // 2 * edge_labels * src_labels * dst_labels
    unsigned *scan_keys;
    unsigned *scan_crs;
    unsigned *scan_dst_crs;
    // fwd or bwd, src, edge_label, dst_label
    unsigned *al_keys;
    unsigned *al_crs;
    unsigned long long p1,p2,bs;
    bitset<BITSET_SIZE> *adj_bs;
    vector<unsigned long long> twohop_bs;
    // ある頂点から各頂点の最短距離を計算 (グラフの向きは一旦考慮しない)
    unsigned *dists;

    LabeledGraph() {}
    LabeledGraph(bool b) { debug = b; }

    void load();
    void set_dists(unsigned root_id);
};

#endif