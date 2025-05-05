#ifndef INFLUENCE_H
#define INFLUENCE_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <omp.h>
#include <algorithm>

#include "load_graph.h"

using namespace std;

unordered_map<int, double> computeInfluenceScores(
    const unordered_map<int, vector<Edge>>& subgraph) {
    
    // Step 1: Precompute neighbor sets
    unordered_map<int, unordered_set<int>> neighbors;
    for (const auto& [node, edges] : subgraph) {
        unordered_set<int> nbrs;
        for (const auto& e : edges) {
            nbrs.insert(e.neighbor);
        }
        neighbors[node] = nbrs;
    }

    // Step 2: Prepare keys and values separately for safe OpenMP parallel loop
    vector<int> nodes;
    for (const auto& [node, _] : subgraph) {
        nodes.push_back(node);
    }

    unordered_map<int, double> scores;

    // Step 3: Parallel score computation
    #pragma omp parallel
    {
        unordered_map<int, double> local_scores;

        #pragma omp for nowait
        for (int i = 0; i < nodes.size(); i++) {
            int node = nodes[i];
            const auto& edges = subgraph.at(node);

            double score = 0.0;

            const auto& node_neighbors = neighbors[node];

            for (const auto& edge : edges) {
                int nbr = edge.neighbor;
                int weight = edge.weight;

                // Skip if neighbor's neighbors are not known
                if (!neighbors.count(nbr)) continue;

                const auto& nbr_neighbors = neighbors[nbr];

                // Compute Jaccard similarity
                int intersection = 0;
                for (int u : node_neighbors) {
                    if (nbr_neighbors.count(u)) {
                        intersection++;
                    }
                }

                int union_size = node_neighbors.size() + nbr_neighbors.size() - intersection;
                double jaccard = union_size > 0 ? static_cast<double>(intersection) / union_size : 0.0;

                // Fallbacck
                if (jaccard == 0.0){
                    //jaccard = 1.0 / (node_neighbors.size() + nbr_neighbors.size());
                    //jaccard = 1.0 / (1 + std::abs(static_cast<int>(node_neighbors.size()) - static_cast<int>(nbr_neighbors.size())));
                    jaccard = 1.0/(node_neighbors.size() + nbr_neighbors.size());
                    jaccard /= 49;
                }
                score += weight * jaccard;
            }

            local_scores[node] = score;
        }

        // Step 4: Merge local scores into global map
        #pragma omp critical
        {
            for (const auto& [node, score] : local_scores) {
                scores[node] = score;
            }
        }
    }

    return scores;
}

#endif
