#include <gtest/gtest.h>
#include <graph.h>
#include <algorithms/shortest_paths/dijkstra_algorithm.h>
#include <unordered_map>

using GraphWeights = std::unordered_map<int, std::unordered_map<int, float>>;

TEST(test_single_source_dijkstra, no_cutoff) {
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
    std::unordered_map<int, float> result = {
        {2, 3},
        {3, 1},
        {4, 5},
        {5, 4},
        {6, 4},
        {7, 5},
        {8, 1},
        {9, 2},
        {1, 0}
    };
    int start = 1;
    const float dist_thr = 5'000;
    const float tol = 0.1;
    graph::Graph<int> graph(std::move(weights));
    auto cutoff = [dist_thr](float dist) { return dist <= dist_thr; };
    auto paths = graph::algorithms::single_source_dijkstra(graph, start, cutoff);

    for (auto& pair : paths) {
        int node_id = pair.first;
        float dist_to_node = pair.second;

        EXPECT_NO_THROW(result.at(node_id));
        EXPECT_NEAR(result.at(node_id), dist_to_node, tol);
    }
}

TEST(test_single_source_dijkstra, with_cutoff) {
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
    std::unordered_map<int, float> result = {
        {2, 3},
        {3, 1},
        // {4, 5},
        // {5, 4},
        // {6, 4},
        // {7, 5},
        {8, 1},
        {9, 2},
        {1, 0}
    };
    int start = 1;
    const float dist_thr = 3;
    const float tol = 0.1;
    graph::Graph<int> graph(std::move(weights));
    auto cutoff = [dist_thr](float dist) { return dist <= dist_thr; };
    auto paths = graph::algorithms::single_source_dijkstra(graph, start, cutoff);

    for (auto& pair : paths) {
        int node_id = pair.first;
        float dist_to_node = pair.second;

        EXPECT_NE(result.find(node_id), result.end());
        EXPECT_NEAR(result.at(node_id), dist_to_node, tol);
    }
}