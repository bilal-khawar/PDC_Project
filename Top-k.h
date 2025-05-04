#ifndef TOPK_H
#define TOPK_H

#include <queue>
#include <utility>

vector<pair<int, double>> getTopKInfluencers(const unordered_map<int, double>& scores, int k) {
    // Min-heap to maintain top-k (smallest score at top)
    auto cmp = [](const pair<int, double>& a, const pair<int, double>& b) {
        return a.second > b.second; // min-heap based on score
    };
    priority_queue<pair<int, double>, vector<pair<int, double>>, decltype(cmp)> minHeap(cmp);

    for (const auto& [node, score] : scores) {
        if (minHeap.size() < k) {
            minHeap.emplace(node, score);
        } else if (score > minHeap.top().second) {
            minHeap.pop();
            minHeap.emplace(node, score);
        }
    }

    // Move to vector and sort in descending order
    vector<pair<int, double>> topK;
    while (!minHeap.empty()) {
        topK.push_back(minHeap.top());
        minHeap.pop();
    }

    reverse(topK.begin(), topK.end()); // Highest score first
    return topK;
}


#endif