#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace geo_utils {
namespace geohash {
    constexpr double kLatIntervalMin = -90.0;
    constexpr double kLatIntervalMax = 90.0;
    constexpr double kLonIntervalMin = -180.0;
    constexpr double kLonIntervalMax = 180.0;

    inline std::string xy2hash(int x, int y, size_t dim) {
        // TODO
        return std::string();
    }

    inline std::string encode_int(std::string& code, int bits_per_char) {
        // TODO
        return std::string();
    }

    inline std::pair<int, int> coord2int(double lon, double lat, int dim) {
        if (dim < 1) {
            throw std::runtime_error("Dim must be >= 1: " + std::to_string(dim));
        }

        auto lat_y = (lat + kLatIntervalMax) / 180.0 * dim;
        auto lon_x = (lon + kLonIntervalMax) / 360.0 * dim;

        return {std::min<int>(dim - 1, lon_x), std::min<int>(dim - 1, lat_y)};
    }

    inline std::string encode(double lon, double lat, int precision = 18, int bits_per_char = 2) {
        if (lon < kLonIntervalMin || lon > kLonIntervalMax || lat < kLatIntervalMin || lat > kLatIntervalMax) {
            throw std::runtime_error("Wrong lon or lat: " + std::to_string(lon) + " " + std::to_string(lat));
        }

        if (precision <= 0) {
            throw std::runtime_error("Wrong precision: " + std::to_string(precision));
        }

        if (bits_per_char != 2 && bits_per_char != 4 && bits_per_char != 6) {
            throw std::runtime_error("Wrong bits_per_char: " + std::to_string(bits_per_char));
        }

        int bits = precision * bits_per_char;
        int level = bits >> 1;
        std::uint64_t dim = 1ull << level;

        auto xy = coord2int(lon, lat, dim);
        auto code = xy2hash(xy.first, xy.second, dim);
        std::string hash = encode_int(code, bits_per_char);
        
        if (precision > hash.size()) {
            return std::string(precision - hash.size(), '0') + hash;
        }
        else {
            return hash;
        }
    }
}
}