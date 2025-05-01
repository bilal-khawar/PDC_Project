from collections import defaultdict
import os

def convert_weighted_edgelist_to_metis(input_file, output_file):
    """
    Convert a weighted edge list to METIS format, ensuring correct edge count.
    
    Edge list format: source_id target_id weight
    METIS format: First line: <num_nodes> <num_edges> 1
                  Following lines: list of "target weight" pairs for each node
    """
    # Track edges and nodes
    edges = set()
    nodes = set()
    
    # Read the edge list file
    print(f"Reading edge list from {input_file}...")
    with open(input_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
                
            u = int(parts[0])
            v = int(parts[1])
            w = int(parts[2]) if len(parts) > 2 else 1  # Default weight of 1 if not specified
            
            # Skip self-loops
            if u == v:
                continue
                
            # Add both nodes to our node set
            nodes.add(u)
            nodes.add(v)
            
            # Add edge to our edge set (undirected, so store in canonical order)
            edges.add((min(u, v), max(u, v), w))
    
    # Create node mapping (original ID -> METIS ID starting from 1)
    node_map = {node: idx + 1 for idx, node in enumerate(sorted(nodes))}
    reverse_map = {v: k for k, v in node_map.items()}
    
    # Build adjacency list using METIS node IDs
    adj_list = defaultdict(list)
    for u, v, w in edges:
        u_metis = node_map[u]
        v_metis = node_map[v]
        # For undirected graph, add both directions
        adj_list[u_metis].append((v_metis, w))
        adj_list[v_metis].append((u_metis, w))
    
    # Write to METIS format
    print(f"Writing METIS format to {output_file}...")
    with open(output_file, 'w') as f:
        # First line: num_nodes num_edges 1 (1 indicates weighted)
        f.write(f"{len(nodes)} {len(edges)} 1\n")
        
        # For each node in order
        for i in range(1, len(nodes) + 1):
            if i in adj_list:
                # Sort by target node ID for consistency
                neighbors = sorted(adj_list[i])
                # Format: "target1 weight1 target2 weight2 ..."
                neighbor_str = " ".join(f"{v} {w}" for v, w in neighbors)
                f.write(f"{neighbor_str}\n")
            else:
                # Isolated node
                f.write("\n")
    
    print(f"Conversion complete!")
    print(f"Nodes: {len(nodes)}, Edges: {len(edges)}")
    
    # Save node mapping for future reference
    mapping_file = f"{output_file}.mapping.txt"
    with open(mapping_file, 'w') as f:
        for orig_id, metis_id in node_map.items():
            f.write(f"{orig_id} {metis_id}\n")
    
    print(f"Node mapping saved to {mapping_file}")
    return node_map, reverse_map

def convert_unweighted_edgelist_to_metis(input_file, output_file):
    """
    Convert an unweighted edge list to METIS format, ensuring correct edge count.
    
    Edge list format: source_id target_id
    METIS format: First line: <num_nodes> <num_edges>
                  Following lines: list of targets for each node
    """
    # Track edges and nodes
    edges = set()
    nodes = set()
    
    # Read the edge list file
    print(f"Reading edge list from {input_file}...")
    with open(input_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
                
            u = int(parts[0])
            v = int(parts[1])
            
            # Skip self-loops
            if u == v:
                continue
                
            # Add both nodes to our node set
            nodes.add(u)
            nodes.add(v)
            
            # Add edge to our edge set (undirected, so store in canonical order)
            edges.add((min(u, v), max(u, v)))
    
    # Create node mapping (original ID -> METIS ID starting from 1)
    node_map = {node: idx + 1 for idx, node in enumerate(sorted(nodes))}
    reverse_map = {v: k for k, v in node_map.items()}
    
    # Build adjacency list using METIS node IDs
    adj_list = defaultdict(list)
    for u, v in edges:
        u_metis = node_map[u]
        v_metis = node_map[v]
        # For undirected graph, add both directions
        adj_list[u_metis].append(v_metis)
        adj_list[v_metis].append(u_metis)
    
    # Write to METIS format
    print(f"Writing METIS format to {output_file}...")
    with open(output_file, 'w') as f:
        # First line: num_nodes num_edges
        f.write(f"{len(nodes)} {len(edges)}\n")
        
        # For each node in order
        for i in range(1, len(nodes) + 1):
            if i in adj_list:
                # Sort by target node ID for consistency
                neighbors = sorted(adj_list[i])
                # Format: "target1 target2 ..."
                neighbor_str = " ".join(str(v) for v in neighbors)
                f.write(f"{neighbor_str}\n")
            else:
                # Isolated node
                f.write("\n")
    
    print(f"Conversion complete!")
    print(f"Nodes: {len(nodes)}, Edges: {len(edges)}")
    
    # Save node mapping for future reference
    mapping_file = f"{output_file}.mapping.txt"
    with open(mapping_file, 'w') as f:
        for orig_id, metis_id in node_map.items():
            f.write(f"{orig_id} {metis_id}\n")
    
    print(f"Node mapping saved to {mapping_file}")
    return node_map, reverse_map

def check_if_weighted(input_file, num_lines_to_check=100):
    """Check if the edge list file appears to be weighted."""
    with open(input_file, 'r') as f:
        for i in range(num_lines_to_check):
            line = f.readline().strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) >= 3:
                try:
                    # Try to parse the third column as a weight
                    int(parts[2])
                    return True
                except ValueError:
                    pass
    return False

def process_all_files():
    # Create output directory if it doesn't exist
    os.makedirs("graphs", exist_ok=True)
    
    # Process each dataset
    datasets = [
        "higgs-mention_network",
        "higgs-reply_network",
        "higgs-retweet_network"
    ]
    
    for dataset in datasets:
        input_file = f"datasets/{dataset}.edgelist"
        output_file = f"graphs/{dataset}.graph"
        
        if not os.path.exists(input_file):
            print(f"Warning: Input file {input_file} not found. Skipping.")
            continue
        
        print(f"\nProcessing {dataset}...")
        
        # Check if the file appears to be weighted
        is_weighted = check_if_weighted(input_file)
        
        if is_weighted:
            print(f"Detected weighted edge list. Converting with weights...")
            convert_weighted_edgelist_to_metis(input_file, output_file)
        else:
            print(f"Detected unweighted edge list. Converting without weights...")
            convert_unweighted_edgelist_to_metis(input_file, output_file)
        
        print(f"Conversion of {dataset} completed.")
        print(f"You can now run: gpmetis {output_file} 8")

if __name__ == "__main__":
    process_all_files()