#include "../include/planner.hpp"

#include <iostream>
#include <algorithm>
#include <queue>
#include <cmath>


std::vector<int> AggPlanner::get_order() {
    std::vector<int> current_order;
    std::vector<int> intersects(q->vars.size());
    std::unordered_map<int,int> via_map;
    std::unordered_set<int> non_vias;
    std::vector<std::unordered_set<int>> dependencies(q->vars.size());
    min_cost = -1;
    recursive_search(current_order, via_map, non_vias, dependencies, intersects);

    std::cout << "best_order: ";
    for (auto &v : best_order) { std::cout << v << " "; }
    std::cout << std::endl;
    std::cout << "min_cost: " << min_cost << std::endl;
    std::cout << "via_map: ";
    for (auto &e : best_via_map) { std::cout << e.first << "->" << e.second << " "; }
    std::cout << std::endl;

    return best_order;
}


void AggPlanner::recursive_search(std::vector<int> &current_order, std::unordered_map<int,int> &via_map,
    std::unordered_set<int> &non_vias, std::vector<std::unordered_set<int>> &dependencies, std::vector<int> &intersects) {
    // 停止条件: current_orderが全ての頂点を含んでいれば停止
    if (current_order.size() == q->vars.size()) {
        // 処理コスト計算, best_orderと比較
        int current_cost = 0;
        for (int v=0; v<intersects.size(); ++v) {
            if (intersects[v] < 2) { continue; }
            std::queue<int> queue;
            std::unordered_set<int> all_precedents;
            for (auto &v_pre : dependencies[v]) {
                all_precedents.insert(v_pre);
                queue.push(v_pre);
            }
            while (!queue.empty()) {
                int v_pre = queue.front();
                queue.pop();
                for (auto &anc : dependencies[v_pre]) {
                    if (!all_precedents.contains(anc)) {
                        all_precedents.insert(anc);
                        queue.push(anc);
                    }
                }
            }
            current_cost += (intersects[v]-1) * std::pow(10, all_precedents.size()-1);
        }
        if (min_cost == -1 || current_cost < min_cost) {
            best_order = current_order;
            min_cost = current_cost;
            best_via_map = via_map;
        }
        return;
    }

    // current_orderに含まれていない頂点から、current_orderのいずれかの頂点と隣接している頂点を収集
    // exception: まだorderが空の場合は全ての頂点を収集
    std::vector<int> candidates;
    std::vector<std::vector<int>> precedents(q->vars.size());
    if (current_order.size() == 0) {
        for (auto &e : q->vars) { candidates.push_back(e.first); }
    }
    for (auto &e : q->vars) {
        if (std::find(current_order.begin(), current_order.end(), e.first) == current_order.end()) {
            for (int i=0; i<q->triples.size(); ++i) {
                int src = q->triples[i][0]; int dst = q->triples[i][2];
                if (e.first == src && std::find(current_order.begin(), current_order.end(), dst) != current_order.end()) {
                    if (precedents[e.first].size() == 0) { candidates.push_back(e.first); }
                    precedents[e.first].push_back(dst);
                }
                if (e.first == dst && std::find(current_order.begin(), current_order.end(), src) != current_order.end()) {
                    if (precedents[e.first].size() == 0) { candidates.push_back(e.first); }
                    precedents[e.first].push_back(src);
                }
            }
        }
    }
    std::sort(candidates.begin(), candidates.end());
    
    // candidatesを順にorderに追加し、次の頂点を追加するために再帰する
    for (auto &v : candidates) {
        std::vector<int> new_order = current_order;
        new_order.push_back(v);
        std::vector<std::unordered_set<int>> new_org_dependencies = dependencies;
        new_org_dependencies[v].insert(precedents[v].begin(), precedents[v].end());
        std::vector<int> new_intersects = intersects;
        new_intersects[v] = precedents[v].size();
        // 集約隣接リストを用いる場合のためのdependencies
        std::vector<std::unordered_set<int>> new_agg_dependencies = new_org_dependencies;

        // 追加する頂点を以降、集約隣接リストの経由頂点として使えない場合にnon-viasに追加
        std::unordered_set<int> new_non_vias = non_vias;
        if (new_order.size() == 1 || precedents[v].size() > 1) {
            new_non_vias.insert(v);
        }

        // 追加する頂点を得るために集約隣接リストを使用できるかチェック
        std::unordered_map<int,int> new_via_map = via_map;
        for (auto &v_pre : precedents[v]) {
            if (!non_vias.contains(v_pre) && precedents[v].size() > 1) {
                new_via_map[v_pre] = v;
                // dependenciesの入れ替え
                new_agg_dependencies[v_pre].insert(v);
                new_agg_dependencies[v].erase(v_pre);
                new_agg_dependencies[v].insert(*dependencies[v_pre].begin());
            }
            new_non_vias.insert(v_pre);
        }
        // via_mapに新たな頂点が追加された場合は、集約を使わないバージョンも実行
        if (new_via_map.size() != via_map.size()) {
            recursive_search(new_order, via_map, new_non_vias, new_org_dependencies, new_intersects);
        }
        recursive_search(new_order, new_via_map, new_non_vias, new_agg_dependencies, new_intersects);
    }
}


std::vector<int> get_plan(Query *q, LabeledGraph *g, std::string &executor_name) {
    std::vector<int> optimized_order;
    if (executor_name == "agg") {
        // 想定される全ての順序から最適なものを選択
        AggPlanner p = AggPlanner(q,g);
        optimized_order = p.get_order();
    }
    else {
        GraphFlowPlanner p = GraphFlowPlanner(q,g);
        optimized_order = p.get_order();
    }

    return optimized_order;
}
