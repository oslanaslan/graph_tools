#include <cstddef>
#include <numeric>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <algorithms/shortest_paths/dijkstra_algorithm.h>
#include <geo_utils/geohash.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <graph.h>
#include <iostream>
#include <algorithms/parallel.h>

using WeightsMap = std::unordered_map<std::string, std::unordered_map<std::string, float>>;
using SingleSourceDijkstraReturn = std::unordered_map<std::string, float>;
using MultiSourceDijkstraReturn = std::vector<std::unordered_map<std::string, float>>;

SingleSourceDijkstraReturn single_source_dijkstra(WeightsMap weights, std::string start, float dist_cutoff) {
    graph::Graph<std::string> graph(std::move(weights));
    auto cutoff_func = [dist_cutoff](float dist) {
        return dist <= dist_cutoff;
    };
    return graph::algorithms::single_source_dijkstra(graph, start, cutoff_func);
}

MultiSourceDijkstraReturn multi_source_dijkstra(WeightsMap weights, std::vector<std::string> start_vec, float dist_cutoff, int n_threads) {
    graph::Graph<std::string> graph(std::move(weights));
    auto cutoff_func = [dist_cutoff](float dist) {
        return dist <= dist_cutoff;
    };
    return graph::algorithms::multi_source_dijkstra(graph, start_vec, n_threads, cutoff_func);
}

std::vector<std::string> ghash_encode(std::vector<double>& lon_vec, std::vector<double>& lat_vec, int n_threads = 1) {
    std::vector<size_t> point_idx_vec(lon_vec.size());
    auto task = [&lat_vec, &lon_vec](size_t point_idx, int thread_num) {
        double lon = lon_vec.at(point_idx);
        double lat = lat_vec.at(point_idx);
        return geo_utils::geohash::encode(lon, lat);
    };

    std::iota(point_idx_vec.begin(), point_idx_vec.end(), 0);
    auto res = graph::algorithms::run_in_threads(point_idx_vec, n_threads, task);

    return res;
}

PYBIND11_MODULE(graph_utils, graph_utils) {
    graph_utils.doc() = "Graph utils";

    graph_utils.def(
        "single_source_dijkstra",
        &single_source_dijkstra,
        "Single source dijkstra\n"
        "Parameters\n"
        "\tweights: Dict[str, Dict[str, float]]\n"
        "\t\tGraph edge u -> v edge weights\n"
        "\tstart: str\n"
        "\t\tStart vertex id\n"
        "\tdist_cutoff: float\n"
        "\t\tMax dijkstra depth distance cutoff in meters\n"
        "Return\n"
        "\tDict[str, float]\n"
        "\t\tDict of all visited vertices and corresponding shortest paths\n"
    );
    graph_utils.def(
        "multi_source_dijkstra",
        &multi_source_dijkstra,
        "Multi source dijkstra\n"
        "Parameters\n"
        "\tweights: Dict[str, Dict[str, float]]\n"
        "\t\tGraph edge u -> v edge weights\n"
        "\tstart_vec: List[str]\n"
        "\t\tList of start vertices ids\n"
        "\tdist_cutoff: float\n"
        "\t\tMax dijkstra depth distance cutoff in meters\n"
        "\tn_threads: int\n"
        "\t\tNumber of parallel running threads\n"
        "Return\n"
        "\tDict[str, float]\n"
        "\t\tList of dicts of all visited vertices and corresponding shortest paths for all given start vertices\n"
    );
    graph_utils.def(
        "ghash_encode",
        &ghash_encode
    );
}