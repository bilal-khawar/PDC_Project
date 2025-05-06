# METIS Graph Partitioning – Higgs Twitter Dataset

This repository contains partitioned graph data derived from various interaction networks in the Higgs Twitter dataset, using METIS (gpmetis). The goal is to segment each network into communities and analyze structural patterns in online user behavior.

## 📁 Files Overview

### 🔹 Graph Partition Files (`*.part.8`)

These files contain the output of METIS partitioning, where each line corresponds to a node, and the value is the community number (0–7) assigned to that node.

| File Name | Description |
| --- | --- |
| `higgs-retweet_network.graph.part.8` | Partitioned retweet graph |
| `higgs-mention_network.graph.part.8` | Partitioned mention graph |
| `higgs-reply_network.graph.part.8` | Partitioned reply graph |
| `higgs-social_network.graph.part.8` | Partitioned overall social graph |

#### How to Read:
- Line number `i` = METIS-internal node ID `i`
- Value at line `i` = Assigned community ID (from 0 to 7)

⚠️ These IDs are not original Twitter user IDs. Use the mapping files to convert them.

### 🔹 Mapping Files (`*.mapping.txt`)

These map METIS internal node IDs back to the original Twitter user IDs.

Use them to:
- Map partition results back to original users
- Join community labels with other user metadata

### 📊 Community Sizes Visualizations

Each `.jpg` file (e.g., `community_sizes.jpg`) shows the distribution of users per community for a specific interaction type. Use these to quickly identify:
- Dominant clusters
- Skewed partitions
- Outliers

---

## 🧪 Running Influence Algorithms

### 🔸 Serial Version

✅ **Compile:**
```bash
g++ -std=c++17 -O2 -o serial_influence serial_influence.cpp
