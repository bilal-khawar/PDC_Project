from collections import defaultdict

def convert_edgelist_to_metis(input_file, output_file):
    adj = defaultdict(set)

    # Read edge list
    with open(input_file, 'r') as f:
        for line in f:
            if line.strip() == "" or line.startswith("#"):
                continue
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            u, v = map(int, parts[:2])
            if u == v:
                continue  # Skip self-loops
            adj[u].add(v)  # Directed edge u -> v
            adj[v].add(u)  # For METIS, treat as undirected by adding v -> u

    # Remap node IDs to contiguous 1-based IDs
    all_nodes = sorted(adj.keys())
    node_mapping = {node: idx + 1 for idx, node in enumerate(all_nodes)}
    remapped_adj = defaultdict(set)

    for u in adj:
        u_mapped = node_mapping[u]
        for v in adj[u]:
            v_mapped = node_mapping[v]
            remapped_adj[u_mapped].add(v_mapped)

    # Count edges (for undirected graphs, each edge is counted once)
    total_edges = sum(len(neighbors) for neighbors in remapped_adj.values())
    edge_count = total_edges // 2  # Divide by 2 since each edge is counted twice

    # Write METIS .graph file
    with open(output_file, 'w') as f:
        f.write(f"{len(remapped_adj)} {edge_count}\n")
        for i in range(1, len(all_nodes) + 1):
            if i in remapped_adj:
                neighbors = sorted(remapped_adj[i])
                f.write(" ".join(map(str, neighbors)) + "\n")
            else:
                f.write("\n")  # Empty line for isolated nodes

    print(f"Converted {input_file} -> {output_file} with {len(remapped_adj)} nodes and {edge_count} edges.")

# Example usage
convert_edgelist_to_metis("datasets/higgs-social_network.edgelist", "higgs-social_network.graph")