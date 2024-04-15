#include "algorithms/parallel.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/geom/Point.h"
#include "geos/geom/Polygon.h"
#include <__chrono/duration.h>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <geos/io/GeoJSONReader.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <geo_utils/geohash.h>
#include <io/read_file.h>
#include <algorithms/light_polygon.h>

TEST(test_polygon_geohashing, small_polygon) {
    using namespace geos::io;
    using namespace geos::geom;

    // Two polygons, one with hole
    std::string test_geometry_wkt = "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)),((20 35, 10 30, 10 10, 30 5, 45 20, 20 35),(30 20, 20 15, 20 25, 30 20)))";

    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();
    WKTReader wkt_reader(*geometry_factory);
    WKTWriter wkt_writer;
    std::unique_ptr<Geometry> test_polygon(wkt_reader.read(test_geometry_wkt));

    
}

TEST(test_geoms, polygon_ghash_mapping) {
    using namespace geos::geom;
    using namespace geos::io;

    // step = (0.0009, 0.0016)
    // step_g = 4000000
    int n_threads = 12;
    const std::string filename = "test/tests/test_geo_utils/resources/dolgoprudniy_city_polygon.geojson";
    constexpr double lon_step = 0.0016 / 2;
    constexpr double lat_step = 0.0009 / 2;

    GeoJSONReader reader;
    WKTWriter writer;
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();

    // Read file into string and parse geojson
    std::string content = io::read_file(filename);
    auto geom_collection = reader.read(content);
    auto geos_polygon = geom_collection->getGeometryN(0);

    auto bbox = geos_polygon->getEnvelopeInternal();
    double min_lon = bbox->getMinX();
    double max_lon = bbox->getMaxX();
    double min_lat = bbox->getMinY();
    double max_lat = bbox->getMaxY();
    std::vector<double> lon_vec;
    std::vector<double> lat_vec;
    size_t lat_cnt = (max_lat - min_lat) / lat_step;
    size_t lon_cnt = (max_lon - min_lon) / lon_step;

    std::vector<size_t> point_ids_vec(lon_cnt * lat_cnt);
    std::iota(point_ids_vec.begin(), point_ids_vec.end(), 0);

    for (double lon = min_lon; lon <= max_lon; lon += lon_step) {
        for (double lat = min_lat; lat <= max_lat; lat += lat_step) {
            lon_vec.push_back(lon);
            lat_vec.push_back(lat);
        }
    }

    auto filter_points_task = [&lon_vec, &lat_vec, &geos_polygon, &geometry_factory](size_t point_idx, int thread_num) {
        double lon = lon_vec[point_idx];
        double lat = lat_vec[point_idx];
        auto point = geometry_factory->createPoint(Coordinate(lon, lat));
        std::string res = "";

        if (geos_polygon->contains(point.get())) {
            res = geo_utils::geohash::encode(lon, lat);
        }

        return res;
    };
    std::cout << "Start threads" << std::endl;
    auto geohash_vec = graph::algorithms::run_in_threads(point_ids_vec, n_threads, filter_points_task);
}

TEST(test_polygon_geohashing, moscow_geos_polygon) {
    using namespace geos::geom;
    using namespace geos::io;

    // step = (0.0009, 0.0016)
    // step_g = 4000000
    int n_threads = 12;
    const std::string filename = "test/tests/test_geohashing/resources/region_polygons.geojson";
    constexpr double lon_step = 0.0016 / 2;
    constexpr double lat_step = 0.0009 / 2;

    GeoJSONReader reader;
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();

    // Read file into string and parse geojson
    std::string content = io::read_file(filename);
    auto geom_collection = reader.read(content);

    std::cout << geom_collection->getNumGeometries() << std::endl;

    auto geos_polygon = geom_collection->getGeometryN(32);

    std::cout << geos_polygon->getGeometryType() << std::endl;

    auto bbox = geos_polygon->getEnvelopeInternal();
    double min_lon = bbox->getMinX();
    double max_lon = bbox->getMaxX();
    double min_lat = bbox->getMinY();
    double max_lat = bbox->getMaxY();
    std::vector<double> lon_vec;
    std::vector<double> lat_vec;
    size_t lat_cnt = (max_lat - min_lat) / lat_step;
    size_t lon_cnt = (max_lon - min_lon) / lon_step;

    std::vector<size_t> point_ids_vec(lon_cnt * lat_cnt);
    std::iota(point_ids_vec.begin(), point_ids_vec.end(), 0);

    for (double lon = min_lon; lon <= max_lon; lon += lon_step) {
        for (double lat = min_lat; lat <= max_lat; lat += lat_step) {
            lon_vec.push_back(lon);
            lat_vec.push_back(lat);
        }
    }

    auto filter_points_task = [&lon_vec, &lat_vec, &geos_polygon, &geometry_factory](size_t point_idx, int thread_num) {
        double lon = lon_vec[point_idx];
        double lat = lat_vec[point_idx];
        auto point = geometry_factory->createPoint(Coordinate(lon, lat));
        std::string res = "";

        if (geos_polygon->contains(point.get())) {
            res = geo_utils::geohash::encode(lon, lat);
        }

        return res;
    };
    std::cout << "Start threads" << std::endl;
    auto geohash_vec = graph::algorithms::run_in_threads(point_ids_vec, n_threads, filter_points_task);

    auto end_unique_iter = std::unique(geohash_vec.begin(), geohash_vec.end());
    geohash_vec.resize(std::distance(geohash_vec.begin(), end_unique_iter));

    std::cout << geohash_vec.size() << std::endl;

    auto fout  = std::ofstream("test/tests/test_geohashing/resources/moscow_geohashes_geos.txt");

    for (auto ghash : geohash_vec) {
        if (ghash.length() > 0)
            fout << ghash << "\n";
    }

    fout.close();
}

TEST(test_polygon_geohashing, moscow_light_polygon) {
    using namespace geos::geom;
    using namespace geos::io;
    using LightMultiPolygon = graph::algorithms::LightMultiPolygon;

    // step = (0.0009, 0.0016)
    // step_g = 4000000
    int n_threads = 12;
    const std::string filename = "test/tests/test_geohashing/resources/region_polygons.geojson";
    constexpr double lon_step = 0.0016 / 2;
    constexpr double lat_step = 0.0009 / 2;

    GeoJSONReader reader;
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();

    // Read file into string and parse geojson
    std::string content = io::read_file(filename);
    auto geom_collection = reader.read(content);

    std::cout << geom_collection->getNumGeometries() << std::endl;

    auto geos_polygon = geom_collection->getGeometryN(32);

    std::cout << geos_polygon->getGeometryType() << std::endl;

    auto bbox = geos_polygon->getEnvelopeInternal();
    double min_lon = bbox->getMinX();
    double max_lon = bbox->getMaxX();
    double min_lat = bbox->getMinY();
    double max_lat = bbox->getMaxY();
    std::vector<double> lon_vec;
    std::vector<double> lat_vec;
    size_t lat_cnt = (max_lat - min_lat) / lat_step;
    size_t lon_cnt = (max_lon - min_lon) / lon_step;

    std::vector<size_t> point_ids_vec(lon_cnt * lat_cnt);
    std::iota(point_ids_vec.begin(), point_ids_vec.end(), 0);

    for (double lon = min_lon; lon <= max_lon; lon += lon_step) {
        for (double lat = min_lat; lat <= max_lat; lat += lat_step) {
            lon_vec.push_back(lon);
            lat_vec.push_back(lat);
        }
    }

    LightMultiPolygon light_multipolygon(geos_polygon);

    auto filter_points_task = [&](size_t point_idx, int thread_num) {
        double lon = lon_vec[point_idx];
        double lat = lat_vec[point_idx];
        std::string res = "";

        if (light_multipolygon.contains(lon, lat)) {
            res = geo_utils::geohash::encode(lon, lat);
        }

        return res;
    };
    std::cout << "Start threads" << std::endl;
    auto geohash_vec = graph::algorithms::run_in_threads(point_ids_vec, n_threads, filter_points_task);

    auto end_unique_iter = std::unique(geohash_vec.begin(), geohash_vec.end());
    geohash_vec.resize(std::distance(geohash_vec.begin(), end_unique_iter));

    std::cout << geohash_vec.size() << std::endl;

    auto fout  = std::ofstream("test/tests/test_geohashing/resources/moscow_geohashes_light.txt");

    for (auto ghash : geohash_vec) {
        if (ghash.length() > 0)
            fout << ghash << "\n";
    }

    fout.close();
}