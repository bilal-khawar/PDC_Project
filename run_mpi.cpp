// mpic++ -std=c++17 -O2 -o run_mpi run_mpi.cpp
// mpirun --hostfile machinefile -np 8 ./run_mpi

#include <mpi.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include "load_graph.h"
#include "Influence.h"
#include "Top-k.h"

using namespace std;

// Define NUM_PARTS, adjust it based on the number of processes you are running.
#define NUM_PARTS 8

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ensure the number of processes matches NUM_PARTS
    if (size != NUM_PARTS) {
        if (rank == 0) {
            cerr << "This program must be run with " << NUM_PARTS << " processes." << endl;
        }
        MPI_Finalize();
        return 1;
    }

    for (const auto& gtype : graph_types) {
        string base = "higgs-" + gtype + "_network";
        string graph_path = "graphs/" + base + ".graph";
        string part_path = "gparts/" + base + ".graph.part.8";
        string map_path = "gparts/" + base + ".graph.mapping.txt";
        bool use_mapping = (gtype != "social");

        // Check if all required files exist
        if (!filesystem::exists(graph_path) || !filesystem::exists(part_path) ||
            (use_mapping && !filesystem::exists(map_path))) {
            if (rank == 0) {
                cerr << "Missing file(s) for graph type: " << gtype << endl;
            }
            continue;
        }

        // Load the subgraph for the current rank
        vector<unordered_map<int, vector<Edge>>> subgraphs(NUM_PARTS);
        loadGraph(graph_path, part_path, map_path, subgraphs, use_mapping);
        const auto& local_subgraph = subgraphs[rank];

        // Compute influence scores
        unordered_map<int, double> scores = computeInfluenceScores(local_subgraph);

        int k = 10;
        vector<pair<int, double>> localTopK = getTopKInfluencers(scores, k);

        if (rank == 0) {
            // Print local top-k influencers
            cout << "\n[Rank " << rank << "] Local Top-" << k << " Influencers for Graph: " << gtype << endl;
            for (const auto& [node, score] : localTopK) {
                cout << "Node " << node << " -> Score: " << score << endl;
            }
        }

        // Pack localTopK into array for MPI_Gather (flattened: [id, score, id, score...])
        vector<double> packedLocal(2 * k);
        for (int i = 0; i < k; ++i) {
            packedLocal[2 * i] = static_cast<double>(localTopK[i].first); // node ID
            packedLocal[2 * i + 1] = localTopK[i].second; // score
        }

        // Gather all packed local top-k at rank 0
        vector<double> allPacked;
        if (rank == 0) {
            allPacked.resize(2 * k * size); // Resize to hold data from all processes
        }

        int gather_status = MPI_Gather(packedLocal.data(), 2 * k, MPI_DOUBLE,
                                       allPacked.data(), 2 * k, MPI_DOUBLE,
                                       0, MPI_COMM_WORLD);
        if (gather_status != MPI_SUCCESS) {
            cerr << "MPI_Gather failed!" << endl;
            MPI_Finalize();
            return 1;
        }

        // Rank 0: Consolidate and print global top-k influencers
        if (rank == 0) {
            vector<pair<int, double>> allCandidates;
            for (int i = 0; i < size * k; ++i) {
                int node = static_cast<int>(allPacked[2 * i]);
                double score = allPacked[2 * i + 1];
                allCandidates.emplace_back(node, score);
            }

            // Merge duplicates (same node may appear from multiple processes)
            unordered_map<int, double> merged;
            for (const auto& [node, score] : allCandidates) {
                if (merged.count(node))
                    merged[node] = max(merged[node], score); // Keep the highest score
                else
                    merged[node] = score;
            }

            // Get global top-K from merged scores
            auto globalTopK = getTopKInfluencers(merged, k);

            cout << "\n[Rank 0] Global Top-" << k << " Influencers for Graph: " << gtype << endl;
            for (const auto& [node, score] : globalTopK) {
                cout << "Node " << node << " -> Score: " << score << endl;
            }
        }
    }

    MPI_Finalize();
    return 0;
}
