#include <gtest/gtest.h>
#include <algorithms/parallel.h>
#include <vector>
#include <graph.h>

using GraphWeights = std::unordered_map<int, std::unordered_map<int, float>>;

TEST(test_parallel, no_graph) {
    const int test_samples_count = 100'000;
    const int test_iterations_count = 100'00;
    const int n_threads = 16;
    graph::Graph<int> graph;
    std::vector<int> input(test_samples_count);
    std::vector<int> true_res(test_samples_count, test_iterations_count);
    auto task_func = [&graph, test_iterations_count](int itm, int thread_num) {
        int res = itm;

        for (int i = 0; i < test_iterations_count; i++) {
            res++;
        }

        return res;
    };
    auto results = graph::algorithms::run_in_threads(input, n_threads, task_func);
    
    for (int i = 0; i < true_res.size(); i++) {
        EXPECT_EQ(true_res[i], results[i]) << i;
    }
}
