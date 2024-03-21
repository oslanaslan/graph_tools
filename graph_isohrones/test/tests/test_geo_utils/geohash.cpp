#include <algorithms/parallel.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <geo_utils/geohash.h>
#include <cstdint>
#include <utility>
#include <vector>
#include <fstream>
#include "geos/geom.h"

const std::string COORDS_FILENAME = "test/tests/test_geo_utils/resources/test_coords_to_ghash.csv";

TEST(test_geohash, test_encode) {
    double lon, lat;
    std::string code;
    std::fstream stream{COORDS_FILENAME};

    while (stream) {
        stream >> lon >> lat >> code;
        auto res_code = geo_utils::geohash::encode(lon, lat);
        ASSERT_EQ(res_code, code);
    }
}

TEST(test_geohash, benchmark_encode_to_string) {
    double test_lat = 64.1835;
    double test_lon = -51.7216;
    const std::size_t benchmark_run_count = 100'000'000;
    std::vector<std::string> res_vec(benchmark_run_count);

    for (std::size_t i = 0; i < benchmark_run_count; i++) {
        res_vec[i] = geo_utils::geohash::encode(test_lon, test_lat);
    }
}

TEST(test_geohash, benchmark_encode_to_int) {
    double test_lat = 64.1835;
    double test_lon = -51.7216;
    const std::size_t benchmark_run_count = 100'000'000;
    std::vector<std::uint64_t> res_vec(benchmark_run_count);

    for (std::size_t i = 0; i < benchmark_run_count; i++) {
        auto res_code = geo_utils::geohash::encode_to_int(test_lon, test_lat);
    }
}

TEST(test_geohash, benchmark_multithreaded_encode_to_int) {
    int n_threads = 12;
    double test_lat = 64.1835;
    double test_lon = -51.7216;
    const std::size_t benchmark_run_count = 100'000'000;

    std::pair<double, double> point{test_lon, test_lat};
    std::vector<std::pair<double, double>> points_vec(benchmark_run_count, point);
    auto task = [](const std::pair<double, double> point, int thread_num) {
        return geo_utils::geohash::encode_to_int(point.first, point.second);
    };
    auto res = graph::algorithms::run_in_threads(points_vec, n_threads, task);
}

TEST(test_geohash, benchmark_multithreaded_encode_to_string) {
    int n_threads = 12;
    double test_lat = 64.1835;
    double test_lon = -51.7216;
    const std::size_t benchmark_run_count = 100'000'000;

    std::pair<double, double> point{test_lon, test_lat};
    std::vector<std::pair<double, double>> points_vec(benchmark_run_count, point);
    auto task = [](const std::pair<double, double> point, int thread_num) {
        return geo_utils::geohash::encode(point.first, point.second);
    };
    auto res = graph::algorithms::run_in_threads(points_vec, n_threads, task);
}
