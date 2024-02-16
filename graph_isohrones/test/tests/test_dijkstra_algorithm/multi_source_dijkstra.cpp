#include <gtest/gtest.h>
#include <graph.h>
#include <algorithms/shortest_paths/dijkstra_algorithm.h>
#include <cstddef>
#include <unordered_map>
#include <vector>

using GraphWeights = std::unordered_map<int, std::unordered_map<int, float>>;

TEST(test_multi_source_dijkstra, no_cutoff) {
    GraphWeights weights = {
        {1, {
            {2, 3},
            {3, 1},
            {7, 5},
            {8, 1}
        }},
        {2, {
            {1, 3},
            {4, 2}
        }},
        {3, {
            {1, 1},
            {4, 5},
            {5, 3}
        }},
        {4, {
            {2, 2},
            {3, 5}
        }},
        {5, {
            {3, 3}
        }},
        {6, {
            {7, 7},
            {9, 2}
        }},
        {7, {
            {1, 5},
            {6, 7}
        }},
        {8, {
            {1, 1},
            {9, 1}
        }},
        {9, {
            {8, 1},
            {6, 2}
        }}
    };
    std::vector<int> starts_vec = {1, 2, 3};
    int n_threads = 3;
    const float dist_thr = 5'000;
    const float tol = 0.1;
    graph::Graph<int> graph(std::move(weights));
    auto cutoff = [dist_thr](float dist) { return dist <= dist_thr; };
    auto res_paths = graph::algorithms::multi_source_dijkstra(graph, starts_vec, n_threads, cutoff);
    std::vector<std::unordered_map<int, float>> true_res;

    for (auto start : starts_vec) {
        true_res.push_back(graph::algorithms::single_source_dijkstra(graph, start, cutoff));
    }

    EXPECT_EQ(res_paths.size(), true_res.size());

    for (size_t start_idx = 0; start_idx < true_res.size(); start_idx++) {
        auto cur_res_paths = res_paths[start_idx];
        auto cur_true_paths = true_res[start_idx];

        for (auto& pair : cur_res_paths) {
            int node_id = pair.first;
            float dist_to_node = pair.second;

            EXPECT_NO_THROW(cur_true_paths.at(node_id));
            EXPECT_NEAR(cur_true_paths.at(node_id), dist_to_node, tol);
        }
    }
}

TEST(test_multi_source_dijkstra, with_cutoff) {
    GraphWeights weights = {
        {1, {
            {2, 3},
            {3, 1},
            {7, 5},
            {8, 1}
        }},
        {2, {
            {1, 3},
            {4, 2}
        }},
        {3, {
            {1, 1},
            {4, 5},
            {5, 3}
        }},
        {4, {
            {2, 2},
            {3, 5}
        }},
        {5, {
            {3, 3}
        }},
        {6, {
            {7, 7},
            {9, 2}
        }},
        {7, {
            {1, 5},
            {6, 7}
        }},
        {8, {
            {1, 1},
            {9, 1}
        }},
        {9, {
            {8, 1},
            {6, 2}
        }}
    };
    std::vector<int> starts_vec = {1, 2, 3};
    int n_threads = 3;
    const float dist_thr = 3;
    const float tol = 0.1;
    graph::Graph<int> graph(std::move(weights));
    auto cutoff = [dist_thr](float dist) { return dist <= dist_thr; };
    auto res_paths = graph::algorithms::multi_source_dijkstra(graph, starts_vec, n_threads, cutoff);
    std::vector<std::unordered_map<int, float>> true_res;

    for (auto start : starts_vec) {
        true_res.push_back(graph::algorithms::single_source_dijkstra(graph, start, cutoff));
    }

    EXPECT_EQ(res_paths.size(), true_res.size());

    for (size_t start_idx = 0; start_idx < true_res.size(); start_idx++) {
        auto cur_res_paths = res_paths[start_idx];
        auto cur_true_paths = true_res[start_idx];

        for (auto& pair : cur_res_paths) {
            int node_id = pair.first;
            float dist_to_node = pair.second;

            EXPECT_NO_THROW(cur_true_paths.at(node_id));
            EXPECT_NEAR(cur_true_paths.at(node_id), dist_to_node, tol);
        }
    }
}