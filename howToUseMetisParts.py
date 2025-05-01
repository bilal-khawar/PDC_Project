import matplotlib.pyplot as plt
from collections import defaultdict

def read_node_mapping(mapping_file):
    """Read the mapping from original node IDs to METIS node IDs"""
    orig_to_metis = {}
    metis_to_orig = {}
    
    with open(mapping_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 2:
                orig_id = int(parts[0])
                metis_id = int(parts[1])
                orig_to_metis[orig_id] = metis_id
                metis_to_orig[metis_id] = orig_id
    
    return orig_to_metis, metis_to_orig

def read_partitions(partition_file):
    """Read the partition assignments for each node"""
    partitions = []
    with open(partition_file, 'r') as f:
        for line in f:
            partitions.append(int(line.strip()))
    return partitions

def analyze_partitioned_graph(dataset_name, num_partitions=8):
    """Analyze a partitioned graph and show community distribution"""
    base_path = f"gparts/{dataset_name}"
    graph_file = f"{base_path}.graph"
    mapping_file = f"{base_path}.graph.mapping.txt"
    partition_file = f"{base_path}.graph.part.{num_partitions}"
    
    # Read mappings and partitions
    orig_to_metis, metis_to_orig = read_node_mapping(mapping_file)
    partitions = read_partitions(partition_file)
    
    # Group original nodes by partition
    communities = defaultdict(list)
    for metis_id, partition in enumerate(partitions, start=1):
        # Convert METIS ID to original node ID
        if metis_id in metis_to_orig:
            orig_id = metis_to_orig[metis_id]
            communities[partition].append(orig_id)
    
    # Print community statistics
    print(f"\nCommunity analysis for {dataset_name}:")
    print(f"Number of communities: {len(communities)}")
    
    community_sizes = [len(nodes) for nodes in communities.values()]
    print(f"Total nodes in communities: {sum(community_sizes)}")
    print(f"Average community size: {sum(community_sizes) / len(communities):.2f}")
    print(f"Smallest community: {min(community_sizes)} nodes")
    print(f"Largest community: {max(community_sizes)} nodes")
    
    # Display community size distribution
    plt.figure(figsize=(10, 6))
    plt.bar(range(len(community_sizes)), sorted(community_sizes, reverse=True))
    plt.xlabel('Community Rank (by size)')
    plt.ylabel('Number of Nodes')
    plt.title(f'Community Size Distribution - {dataset_name}')
    plt.tight_layout()
    plt.savefig(f"{base_path}_community_sizes.png")
    print(f"Community size distribution saved to {base_path}_community_sizes.png")
    
    return communities

# Example usage
if __name__ == "__main__":
    datasets = [
        "higgs-mention_network",
        "higgs-reply_network", 
        "higgs-retweet_network"
    ]
    
    for dataset in datasets:
        try:
            communities = analyze_partitioned_graph(dataset)
            
            # Print the top 5 nodes from the largest community
            largest_community = max(communities.items(), key=lambda x: len(x[1]))
            comm_id, nodes = largest_community
            print(f"\nLargest community (ID {comm_id}) has {len(nodes)} nodes")
            print(f"Sample nodes from largest community: {sorted(nodes)[:5]}...")
            
        except FileNotFoundError as e:
            print(f"Error analyzing {dataset}: {e}")
            print(f"Make sure to run gpmetis on the graph first!")