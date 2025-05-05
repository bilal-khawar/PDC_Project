// mpic++ -std=c++17 -O2 -o run_mpi run_mpi.cpp
// mpirun --hostfile machinefile -np 8 ./run_mpi

#include <mpi.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include "load_graph.h"
#include "Influence.h"
#include "Top-k.h"

using namespace std;

// Define NUM_PARTS, adjust it based on the number of processes you are running.
#define NUM_PARTS 8
#define K 10

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != NUM_PARTS) {
        if (rank == 0) {
            cerr << "This program must be run with " << NUM_PARTS << " processes." << endl;
        }
        MPI_Finalize();
        return 1;
    }

    // Final aggregated data (only used by Rank 0)
    unordered_map<int, vector<double>> allNodeScores;
    unordered_map<string, int> gtypeIndex = {
        {"mention", 0}, {"retweet", 1}, {"reply", 2}, {"social", 3}
    };

    for (const auto& gtype : graph_types) {
        string base = "higgs-" + gtype + "_network";
        string graph_path = "graphs/" + base + ".graph";
        string part_path = "gparts/" + base + ".graph.part.8";
        string map_path = "gparts/" + base + ".graph.mapping.txt";
        bool use_mapping = (gtype != "social");

        if (!filesystem::exists(graph_path) || !filesystem::exists(part_path) ||
            (use_mapping && !filesystem::exists(map_path))) {
            if (rank == 0) {
                cerr << "Missing file(s) for graph type: " << gtype << endl;
            }
            continue;
        }


        // Load mapping file to get real-world node IDs
        unordered_map<int, int> localToRealWorldMapping;
        if (use_mapping) {
            ifstream mapFile(map_path);
            if (!mapFile.is_open()) {
                if (rank == 0) {
                    cerr << "Error opening mapping file: " << map_path << endl;
                }
                MPI_Finalize();
                return 1;
            }

            int local_id, real_id;
            while (mapFile >> real_id >> local_id) {
                localToRealWorldMapping[local_id] = real_id;
            }
            mapFile.close();
        }

        vector<unordered_map<int, vector<Edge>>> subgraphs(NUM_PARTS);
        loadGraph(graph_path, part_path, map_path, subgraphs, use_mapping);
        const auto& local_subgraph = subgraphs[rank];

        unordered_map<int, double> scores = computeInfluenceScores(local_subgraph);

        // int k = 10;
        vector<pair<int, double>> localTopK = getTopKInfluencers(scores, K);

        if (rank == 0) {
            cout << "\n[Rank " << rank << "] Local Top-" << K << " Influencers for Graph: " << gtype << endl;
            for (const auto& [node, score] : localTopK) {
                cout << "Node " << node << " -> Score: " << score << endl;
            }
        }

        vector<double> packedLocal(2 * K);
        for (int i = 0; i < K; ++i) {
            packedLocal[2 * i] = static_cast<double>(localTopK[i].first);
            packedLocal[2 * i + 1] = localTopK[i].second;
        }

        vector<double> allPacked;
        if (rank == 0) {
            allPacked.resize(2 * K * size);
        }

        int gather_status = MPI_Gather(packedLocal.data(), 2 * K, MPI_DOUBLE,
                                       allPacked.data(), 2 * K, MPI_DOUBLE,
                                       0, MPI_COMM_WORLD);

        if (gather_status != MPI_SUCCESS) {
            cerr << "MPI_Gather failed!" << endl;
            MPI_Finalize();
            return 1;
        }

        if (rank == 0) {
            vector<pair<int, double>> allCandidates;
            for (int i = 0; i < size * K; ++i) {
                int localNode = static_cast<int>(allPacked[2 * i]);
                double score = allPacked[2 * i + 1];
                int realNode = localToRealWorldMapping.count(localNode) ? localToRealWorldMapping[localNode] : localNode;
                allCandidates.emplace_back(realNode, score);
            }

            unordered_map<int, double> merged;
            for (const auto& [node, score] : allCandidates) {
                merged[node] = max(merged[node], score);
            }

            auto globalTopK = getTopKInfluencers(merged, K);

            cout << "\n[Rank 0] Global Top-" << K << " Influencers for Graph: " << gtype << endl;
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
    }

    // After all graphs processed, compute final weighted score
    if (rank == 0) {
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

        // int final_k = 10;
        cout << "\n========== FINAL GLOBAL TOP-" << K << " INFLUENCERS ==========\n";
        for (int i = 0; i < min(K, (int)finalScores.size()); ++i) {
            int node = finalScores[i].first;
            double score = finalScores[i].second;
            const auto& vec = allNodeScores[node];
            cout << "Node " << node << " -> Overall Score: " << score << endl;
                 //<< " [Mention: " << vec[0] << ", Retweet: " << vec[1]
                 //<< ", Reply: " << vec[2] << ", Social: " << vec[3] << "]\n";
        }
    }

    MPI_Finalize();
    return 0;
}
