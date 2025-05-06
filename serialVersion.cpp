// serial_influence.cpp
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include "load_graph.h"
#include "Influence.h"
#include "Top-k.h"

using namespace std;

const int K = 10;

int main() {
    unordered_map<int, vector<double>> allNodeScores;
    unordered_map<string, int> gtypeIndex = {
        {"mention", 0}, {"retweet", 1}, {"reply", 2}, {"social", 3}
    };

    auto start_time = chrono::high_resolution_clock::now();

    for (const auto& gtype : graph_types) {
        string base = "higgs-" + gtype + "_network";
        string graph_path = "graphs/" + base + ".graph";
        string part_path = "gparts/" + base + ".graph.part.8";
        string map_path = "gparts/" + base + ".graph.mapping.txt";
        bool use_mapping = (gtype != "social");

        if (!filesystem::exists(graph_path) || !filesystem::exists(part_path) ||
            (use_mapping && !filesystem::exists(map_path))) {
            cerr << "Missing file(s) for graph type: " << gtype << endl;
            continue;
        }

        // Load mapping file
        unordered_map<int, int> localToRealWorldMapping;
        if (use_mapping) {
            ifstream mapFile(map_path);
            if (!mapFile.is_open()) {
                cerr << "Error opening mapping file: " << map_path << endl;
                return 1;
            }

            int local_id, real_id;
            while (mapFile >> real_id >> local_id) {
                localToRealWorldMapping[local_id] = real_id;
            }
            mapFile.close();
        }

        // Process all partitions sequentially
        vector<unordered_map<int, vector<Edge>>> subgraphs(NUM_PARTS);
        loadGraph(graph_path, part_path, map_path, subgraphs, use_mapping);

        unordered_map<int, double> mergedScores;
        for (int part = 0; part < NUM_PARTS; ++part) {
            const auto& local_subgraph = subgraphs[part];
            auto scores = computeInfluenceScores(local_subgraph);

            for (const auto& [node, score] : scores) {
                int realNode = localToRealWorldMapping.count(node) ? 
                              localToRealWorldMapping[node] : node;
                mergedScores[realNode] = max(mergedScores[realNode], score);
            }
        }

        auto globalTopK = getTopKInfluencers(mergedScores, K);

        cout << "\nGlobal Top-" << K << " Influencers for Graph: " << gtype << endl;
        for (const auto& [node, score] : globalTopK) {
            cout << "Node " << node << " -> Score: " << score << endl;
        }

        // Update global allNodeScores map
        int idx = gtypeIndex[gtype];
        for (const auto& [node, score] : globalTopK) {
            if (!allNodeScores.count(node)) {
                allNodeScores[node] = vector<double>(4, 0.0);
            }
            allNodeScores[node][idx] = score;
        }
    }

    // Compute final weighted score
    vector<double> weights = {0.3, 0.5, 0.4, 0.01}; // mention, retweet, reply, social
    vector<pair<int, double>> finalScores;

    for (const auto& [node, vec] : allNodeScores) {
        double overall = 0.0;
        for (int i = 0; i < 4; ++i) {
            overall += weights[i] * vec[i];
        }
        finalScores.emplace_back(node, overall);
    }

    sort(finalScores.begin(), finalScores.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    cout << "\n========== FINAL GLOBAL TOP-" << K << " INFLUENCERS ==========\n";
    for (int i = 0; i < min(K, (int)finalScores.size()); ++i) {
        int node = finalScores[i].first;
        double score = finalScores[i].second;
        cout << "Node " << node << " -> Overall Score: " << score << endl;
    }

    cout << "\nTotal execution time: " << duration.count() / 1000.0 << " seconds" << endl;

    return 0;
}