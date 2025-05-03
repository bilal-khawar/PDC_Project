#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>

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
               vector<unordered_map<int, vector<Edge>>>& subgraphs) {
    
    // Load partition file
    vector<int> node_to_partition;
    {
        ifstream in(part_file);
        int p;
        while (in >> p) {
            node_to_partition.push_back(p);
        }
    }

    // Load mapping file: METIS ID → original ID
    vector<int> metis_to_original;
    {
        ifstream in(mapping_file);
        int orig, metis;
        while (in >> orig >> metis) {
            if (metis >= metis_to_original.size()) {
                metis_to_original.resize(metis + 1);
            }
            metis_to_original[metis] = orig;
        }
    }

    // Load graph file and distribute edges into subgraphs
    ifstream infile(graph_file);
    string line;
    int node_id = 0;

    getline(infile, line); // skip header
    while (getline(infile, line)) {
        node_id++;
        istringstream iss(line);
        int neighbor, weight;
        int metis_id = node_id - 1;
        if (metis_id >= node_to_partition.size()) continue;

        int partition = node_to_partition[metis_id];
        int orig_node = metis_to_original.size() > metis_id ? metis_to_original[metis_id] : metis_id;

        while (iss >> neighbor >> weight) {
            int metis_nbr = neighbor - 1;
            int orig_nbr = metis_to_original.size() > metis_nbr ? metis_to_original[metis_nbr] : metis_nbr;
            subgraphs[partition][orig_node].push_back({orig_nbr, weight});
        }
    }
}

int main() {
    for (const auto& gtype : graph_types) {
        string base = "higgs-" + gtype + "_network";
        string graph_path = "graphs/" + base + ".graph";
        string part_path = "gparts/" + base + ".graph.part.8";
        string map_path = "gparts/" + base + ".graph.mapping.txt";

        vector<unordered_map<int, vector<Edge>>> subgraphs(NUM_PARTS);
        loadGraph(graph_path, part_path, map_path, subgraphs);

        cout << "\n====== Loaded graph type: " << gtype << " ======\n";
        for (int i = 0; i < NUM_PARTS; ++i) {
            cout << "Subgraph " << i << " has " << subgraphs[i].size() << " nodes." << endl;
        }

        // Optional: Uncomment to print each subgraph
        
        // for (int i = 0; i < NUM_PARTS; ++i) {
        //     printSubgraph(i, subgraphs[i]);
        // }
        
    }

    return 0;
}
