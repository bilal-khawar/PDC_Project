#ifndef LOADGRAPH_H
#define LOADGRAPH_H


#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

using namespace std;

const vector<string> graph_types = {"mention", "retweet", "reply", "social"};
const int NUM_PARTS = 8;

struct Edge {
    int neighbor;
    int weight;
};

void printSubgraph(int part_id, const unordered_map<int, vector<Edge>>& subgraph) {
    cout << "\n--- Subgraph " << part_id << " ---" << endl;
    for (const auto& [node, edges] : subgraph) {
        cout << "Node " << node << " -> ";
        for (const auto& edge : edges) {
            cout << "(" << edge.neighbor << ", w=" << edge.weight << ") ";
        }
        cout << endl;
    }
}

void loadGraph(const string& graph_file,
               const string& part_file,
               const string& mapping_file,
               vector<unordered_map<int, vector<Edge>>>& subgraphs,
               bool use_mapping) {
    
    // Load partition file
    vector<int> node_to_partition;
    {
        ifstream in(part_file);
        if (!in) {
            cerr << "Error: Cannot open partition file: " << part_file << endl;
            return;
        }
        
        int p;
        while (in >> p) {
            if (p < 0 || p >= NUM_PARTS) {
                cerr << "Warning: Invalid partition " << p << " (should be 0-" << (NUM_PARTS-1) << ")" << endl;
                p = 0; // Default to partition 0 if invalid
            }
            node_to_partition.push_back(p);
        }
        
//        cout << "Loaded " << node_to_partition.size() << " partition assignments from " << part_file << endl;
    }

   
    // Maps metis node ID to original node ID
    unordered_map<int, int> metis_to_original;
    if (use_mapping) {
        ifstream in(mapping_file);
        if (!in) {
            cerr << "Error: Cannot open mapping file: " << mapping_file << endl;
            return;
        }
        
        int orig, metis;
        while (in >> orig >> metis) {
            metis_to_original[metis] = orig;
        }
//        cout << "Loaded " << metis_to_original.size() << " mappings from " << mapping_file << endl;
    }

    // Load graph
    ifstream infile(graph_file);
    if (!infile) {
        cerr << "Error: Cannot open graph file: " << graph_file << endl;
        return;
    }
    
    string line;
    
    // Parse the first line (metadata)
    getline(infile, line);
    istringstream meta(line);
    int total_nodes, total_edges;
    meta >> total_nodes >> total_edges;
//    cout << "Graph has " << total_nodes << " nodes and " << total_edges << " edges" << endl;
    
    // Process each node's adjacency list
    int node_id = 0;
    while (getline(infile, line)) {
        node_id++; // 1-based node ID in the .graph file
        
        // Skip if we don't have partition information for this node
        if (node_id > node_to_partition.size()) {
            cerr << "Warning: No partition info for node " << node_id << ", skipping" << endl;
            continue;
        }
        
        // Get partition for this node from the .part.8 file
        int partition = node_to_partition[node_id-1]; // 0-based indexing for partition array
        
        // Get original node ID using mapping
        int orig_node;
        if (use_mapping) {
            // For non-social graphs, use the mapping file
            auto it = metis_to_original.find(node_id);
            if (it != metis_to_original.end()) {
                orig_node = it->first;
            } else {
                // If no mapping exists, use node_id as fallback
                orig_node = node_id;
            }
        } else {
            // For social graph, node_id is already the original ID
            orig_node = node_id;
        }

        // Parse neighbors for this node
        istringstream iss(line);
        string nbr_line = line;
        
        int neighbor, weight;
        while (iss >> neighbor) {
            if (use_mapping) {
                // For mention/retweet/reply graphs, read the weight
                iss >> weight;
            } else {
                // For social graph, weight is always 1
                weight = 1;
            }
            
            // Map neighbor ID if needed
            int orig_nbr;
            if (use_mapping) {
                auto it = metis_to_original.find(neighbor);
                if (it != metis_to_original.end()) {
                    orig_nbr = it->first;
                } else {
                    orig_nbr = neighbor;
                }
            } else {
                orig_nbr = neighbor;
            }
            
            // Add edge to the appropriate subgraph based on partition
            subgraphs[partition][orig_node].push_back({orig_nbr, weight});
        }
    }
}

// int main() {
//     for (const auto& gtype : graph_types) {
//         string base = "higgs-" + gtype + "_network";
//         string graph_path = "graphs/" + base + ".graph";
//         string part_path = "gparts/" + base + ".graph.part.8";
//         string map_path = "gparts/" + base + ".graph.mapping.txt";

//         bool use_mapping = (gtype != "social");

//         cout << "\n====== Processing graph type: " << gtype << " ======\n";
        
//         // Check if the required files exist
//         if (!filesystem::exists(graph_path)) {
//             cerr << "Error: Graph file not found at " << graph_path << endl;
//             continue;
//         }
        
//         if (!filesystem::exists(part_path)) {
//             cerr << "Error: Partition file not found at " << part_path << endl;
//             continue;
//         }
        
//         if (use_mapping && !filesystem::exists(map_path)) {
//             cerr << "Error: Mapping file not found at " << map_path << endl;
//             continue;
//         }

//         vector<unordered_map<int, vector<Edge>>> subgraphs(NUM_PARTS);
//         loadGraph(graph_path, part_path, map_path, subgraphs, use_mapping);

//         cout << "\n====== Loaded graph type: " << gtype << " ======\n";
//         for (int i = 0; i < NUM_PARTS; ++i) {
//             cout << "Subgraph " << i << " has " << subgraphs[i].size() << " nodes." << endl;
//         }

//         // if (gtype == "mention") {
//         //     for (int i = 0; i < NUM_PARTS; ++i) {
//         //         printSubgraph(i, subgraphs[i]);
//         //     }
//         // }
//     }

//     return 0;
// }



#endif