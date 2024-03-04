#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

// TODO add decode

namespace geo_utils {
namespace geohash {
    constexpr double kLatIntervalMin = -90.0;
    constexpr double kLatIntervalMax = 90.0;
    constexpr double kLonIntervalMin = -180.0;
    constexpr double kLonIntervalMax = 180.0;
    constexpr int kDefaultPrecision = 18;
    constexpr int kDefaultBitsPerChar = 2;

    /**
     * @brief Rotate and flip a quadrant appropriately
     * Based on the implementation here:
     *   https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503
     *
     * @param n
     * @param x
     * @param y
     * @param rx
     * @param ry
     */
    inline std::pair<std::uint64_t, std::uint64_t> rotate(std::uint64_t n, std::uint64_t x, std::uint64_t y, std::uint64_t rx, std::uint64_t ry) {
        if (!ry) {
            if (rx) {
                x = n - 1 - x;
                y = n - 1 - y;
            }

            return std::pair<std::uint64_t, std::uint64_t>{y, x};
        }

        return std::pair<std::uint64_t, std::uint64_t>{x, y};
    }

    /**
     * @brief Convert (x, y) to hashcode.
     * Based on the implementation here:
     *    https://en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=797332503
     *
     * @param x x value of point [0, dim) in dim x dim coord system
     * @param y value of point [0, dim) in dim x dim coord system
     * @param dim Number of coding points each x, y value can take
     * @return uint64_t hash code
     */
    inline std::uint64_t xy2hash(std::uint64_t x, std::uint64_t y, size_t dim) {
        std::uint64_t d = 0;
        std::uint64_t lvl = dim >> 1;

        while (lvl > 0) {
            bool rx = (x & lvl) > 0;
            bool ry = (y & lvl) > 0;
            d += lvl * lvl * ((3 * rx) ^ ry);
            auto xy = rotate(lvl, x, y, rx, ry);
            x = xy.first;
            y = xy.second;
            lvl >>= 1;
        }

        return d;
    }

    /**
     * @brief Convert lon, lat values into a dim x dim-grid coordinate system.
     * 
     * @param lon Longitude value of coordinate (-180.0, 180.0); corresponds to X axis
     * @param lat Latitude value of coordinate (-90.0, 90.0); corresponds to Y axis
     * @param dim Number of coding points each x, y value can take.
     *                  Corresponds to 2^level of the hilbert curve.
     * @return Pair of converted coord ids
     */
    inline std::pair<int, int> coord2int(double lon, double lat, int dim) {
        if (dim < 1) {
            throw std::runtime_error("Dim must be >= 1: " + std::to_string(dim));
        }

        auto lat_y = (lat + kLatIntervalMax) / 180.0 * dim;
        auto lon_x = (lon + kLonIntervalMax) / 360.0 * dim;

        return {std::min<int>(dim - 1, lon_x), std::min<int>(dim - 1, lat_y)};
    }

    /**
     * @brief Encode lat/lon to uint64_t code
     * 
     * @param lon Longitude; between -180.0 and 180.0; WGS 84
     * @param lat Latitude; between -90.0 and 90.0; WGS 84
     * @param precision The number of characters in a geohash
     * @param bits_per_char The number of bits per coding character
     * @return Geohash code in bits representation contained in uint64_t variable
     */
    inline std::uint64_t encode_to_int(double lon, double lat, int precision = kDefaultPrecision, int bits_per_char = kDefaultBitsPerChar) {
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
        std::uint64_t code = xy2hash(xy.first, xy.second, dim);

        return code;
    }

    inline std::string encode_int64(std::uint64_t code, int precision) {
        // TODO
        throw std::runtime_error("Not implemented");
    }

    inline std::string encode_int16(std::uint64_t code, int precision) {
        // TODO
        throw std::runtime_error("Not implemented");
    }
    inline std::string encode_int4(std::uint64_t code, int precision) {
        std::string str_code(precision, '0');

        while (code) {
            str_code[precision - 1] = '0' + (code & 0b11u);
            precision--;
            code >>= 2;
        }

        return str_code;
    }

    /**
     * @brief Convert uint64_t code to string representation
     * 
     * @param code Geohash code
     * @param precision The number of characters in a geohash
     * @param bits_per_char The number of bits per coding character
     * @return Geohash code as a string
     */
    inline std::string code_to_string(std::uint64_t code, int bits_per_char, int precision) {
        if (bits_per_char == 6) {
            return encode_int64(code, precision);
        } else if (bits_per_char == 4) {
            return encode_int16(code, precision);
        } else if (bits_per_char == 2) {
            return encode_int4(code, precision);
        }
        throw std::runtime_error("Bits per char can be only 6, 4, 2");
    }

    /**
     * @brief Encode lat/lon to string code
     * 
     * @param lon Longitude; between -180.0 and 180.0; WGS 84
     * @param lat Latitude; between -90.0 and 90.0; WGS 84
     * @param precision The number of characters in a geohash
     * @param bits_per_char The number of bits per coding character
     * @return Geohash code as a string
     */
    inline std::string encode(double lon, double lat, int precision = kDefaultPrecision, int bits_per_char = kDefaultBitsPerChar) {
        std::uint64_t code = encode_to_int(lon, lat, precision, bits_per_char);
        std::string str_code = code_to_string(code, bits_per_char, precision);

        return str_code;
    }
}
}