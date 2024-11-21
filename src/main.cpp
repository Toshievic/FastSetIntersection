#include "../include/labeled_graph.hpp"
#include "../include/query.hpp"
#include "../include/util.hpp"
#include "../include/executor.hpp"


int main(int argc, char* argv[]) {
    // 引数としてintersectionを行う際の手法をとる
    string method_name = "multiway";
    if (argc > 1) { method_name = argv[1]; }
    bool debug = false;

    LabeledGraph lg(debug);

    string query_sets = "test";

    lg.load();
    vector<Query> queries = load_queries(query_sets, debug);
    for (auto &query : queries) {
        for (int i=0; i<3; ++i) {
            vector<unsigned*> stats;
            // GenericJoin executor(debug, &stats);
            // AlphaGenericJoin executor(debug, &stats);
            // BetaGenericJoin executor(debug, &stats);
            GammaGenericJoin executor(debug, &stats);
            executor.decide_plan(&lg, &query);
            executor.run(method_name);

            // string output_file_path = "../result_2/" + get_timestamp() + ".csv";
            // export_summaries(stats, output_file_path);
        }
    }
}