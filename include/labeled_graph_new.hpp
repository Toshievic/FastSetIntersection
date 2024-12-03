#ifndef LABELED_GRAPH_NEW_HPP
#define LABELED_GRAPH_NEW_HPP

#include <string>
#include <unordered_map>


class LabeledGraph {
public:
    // fwd or bwd, (src_label)-[edge_label]-(dst_label)を満たすエッジを全scanする用
    // 2 * edge_labels * src_labels * dst_labels
    unsigned *scan_v_crs;
    unsigned *scan_srcs;
    unsigned *scan_src_crs;
    unsigned *scan_dst_crs;

    // fwd or bwd, src, edge_label, dst_label
    unsigned *al_v_crs;
    unsigned *al_e_crs;

    unsigned *agg_2hop_v_crs;
    unsigned *agg_2hop_e_crs;
    unsigned *agg_al_v_crs;
    unsigned *agg_al_e_crs;
    
    std::unordered_map<std::string, int> graph_info;

    LabeledGraph() {}
    LabeledGraph(std::string &data_dirpath);
};


class LabeledAggGraph : public LabeledGraph {
public:
    LabeledAggGraph() {}
    LabeledAggGraph(std::string &data_dirpath);
};

#endif