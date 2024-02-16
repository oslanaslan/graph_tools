#pragma once
#include <graph.h>
#include <cstddef>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <graph.h>
#include <algorithms/parallel.h>

namespace graph {
namespace algorithms {
template <typename Vertex, typename Cutoff>
std::unordered_map<Vertex, float> single_source_dijkstra(const Graph<Vertex>& graph, const Vertex& start, Cutoff&& cutoff) {
    std::unordered_map<Vertex, float> ans;
    std::priority_queue<std::pair<Vertex, float>, std::vector<std::pair<Vertex, float>>, std::greater<std::pair<Vertex, float>>> queue;

    ans[start] = 0;
    queue.push({start, 0});

    while (!queue.empty()) {
        std::pair<Vertex, float> current = queue.top();
        queue.pop();

        Vertex v = current.first;
        float dst = current.second;

        if (ans[v] < dst) {
            continue;
        }

        for (std::pair<Vertex, float> e: graph.get_neighbors(v)) {
            Vertex u = e.first;
            float len_vu = e.second;
            float n_dst = dst + len_vu;

            if (!cutoff(n_dst)) {
                continue;
            }

            if (ans.find(u) == ans.end() || n_dst < ans[u]) {
                ans[u] = n_dst;
                queue.push({u, n_dst});
            }
        }
    }

    return ans;
}

template<typename Vertex, typename Cutoff>
std::vector<std::unordered_map<Vertex, float>> multi_source_dijkstra(
    const graph::Graph<Vertex>& graph,
    const std::vector<Vertex>& starts_vec,
    const int n_threads,
    Cutoff&& cutoff_func
) {
    auto task = [&graph, &cutoff_func](const Vertex& start, int thread_num) {
        return graph::algorithms::single_source_dijkstra(graph, start, cutoff_func);
    };
    return graph::algorithms::run_in_threads(starts_vec, n_threads, task);
}

}  // namespace algorithm
}  // namespace graph
