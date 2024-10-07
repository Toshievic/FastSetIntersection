#ifndef LABELED_GRAPH_HPP
#define LABELED_GRAPH_HPP

#include "master.hpp"

#include <vector>
#include <bitset>
#include <unordered_set>
#include <iostream>

#define BITSET_SIZE 1024


// 2-hop indexで利用するハッシュ関数, 局所性の向上が目的
struct DenseHash {
    const unsigned long long MB = 0xffffffff80000000;
    std::size_t operator()(unsigned long long value) const {
        // 元の頂点ID+dirsの部分のみハッシュし、その値に隣接頂点IDの値を足す
        return hash<unsigned long long>()(value & MB) + (value & (~MB));
    }
};


class LabeledGraph {
private:
    bool debug;
public:
    string dataset_name;
    unsigned num_v, num_vl, num_el, max_degree;
    // fwd or bwd, (src_label)-[edge_label]-(dst_label)を満たすエッジを全scanする用
    // 2 * edge_labels * src_labels * dst_labels
    unsigned *scan_v_crs;
    unsigned *scan_src_crs;
    unsigned *scan_dst_crs;
    // fwd or bwd, src, edge_label, dst_label
    unsigned *al_v_crs;
    unsigned *al_e_crs;
    bitset<BITSET_SIZE> *adj_bs;
    unsigned *twohop_v_crs;
    unsigned *twohop_e_crs;
    unordered_set<unsigned long long> twohop_idx;
    vector<unsigned> twohop_bs;

    LabeledGraph() {}
    LabeledGraph(bool b) { debug = b; }

    void load();
};

#endif