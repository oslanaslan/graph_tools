#include <gtest/gtest.h>
#include <geo_utils/geohash.h>
#include <sys/_types/_size_t.h>
#include <vector>
#include <fstream>

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

    for (std::size_t i = 0; i < benchmark_run_count; i++) {
        auto res_code = geo_utils::geohash::encode(test_lon, test_lat);
    }
}

TEST(test_geohash, benchmark_encode_to_int) {
    double test_lat = 64.1835;
    double test_lon = -51.7216;
    const std::size_t benchmark_run_count = 100'000'000;

    for (std::size_t i = 0; i < benchmark_run_count; i++) {
        auto res_code = geo_utils::geohash::encode_to_int(test_lon, test_lat);
    }
}