#ifndef LABELED_GRAPH_NEW_HPP
#define LABELED_GRAPH_NEW_HPP

#include <string>
#include <unordered_map>


class LabeledGraph {
public:
    unsigned *scan_keys;
    unsigned *scan_crs;

    // fwd or bwd, src, edge_label, dst_label
    unsigned *al_keys;
    unsigned *al_crs;
    unsigned *al_hub_keys;
    unsigned *al_hub_crs;

    unsigned *agg_2hop_keys;
    unsigned *agg_2hop_crs;
    unsigned *agg_al_keys;
    unsigned *agg_al_crs;
    
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