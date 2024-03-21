#include "algorithms/parallel.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/geom/Point.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <geos/io/GeoJSONReader.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>
#include <memory>
#include <numeric>
#include <ostream>
#include <string>
#include <vector>
#include <geo_utils/geohash.h>
#include <io/read_file.h>
#include <algorithms/polygon.h>

// size_t allocations = 0

// std::byte buf[1000000]
// size_t alloc = 0

// void* operator new(size_t sz) {
//     void* res = buf + alloc;
//     alloc += sz;
//     return res;
// }

// void operator delete(void*) noexcept {}

TEST(test_geoms, test_polygons) {
    using namespace geos::geom;
    using namespace geos::io;

    // size_t test_count = 1'000'000;
    size_t test_count = 1'000'000;
    std::string test_multipolygon_wkt = "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)),((20 35, 10 30, 10 10, 30 5, 45 20, 20 35),(30 20, 20 15, 20 25, 30 20)))";
    std::string test_polygon_wkt = "POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))";
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();
    WKTReader wkt_reader(*geometry_factory);
    WKTWriter wkt_writer;
    std::unique_ptr<Geometry> test_polygon(wkt_reader.read(test_polygon_wkt));
    std::unique_ptr<Point> test_point = geometry_factory->createPoint(Coordinate(20, 20));

    // std::cout << wkt_writer.write(test_polygon.get()) << std::endl;
    // std::cout << wkt_writer.write(test_point.get()) << std::endl;
    // std::cout << test_polygon->contains(test_point.get()) << std::endl;

    // std::cout << "Allocs: " << allocations << std::endl;

    // test_polygon->contains(test_point.get());
    volatile int dummy = 0;

    for (size_t i = 0; i < test_count; i++) {
        // size_t old = alloc;
        int r = test_polygon->contains(test_point.get());
        dummy += r;
        // alloc = old;
    }

    EXPECT_NE(dummy, 0);

    // std::cout << "Allocs: " << allocations << std::endl;
}

TEST(test_geoms, test_custom_polygons) {
    using Point = graph::algorithms::Point2d;

    // size_t test_count = 1'000'000;
    constexpr size_t test_count = 1'000'000;

    std::string test_polygon_wkt = "POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))";

    std::vector<Point> points{{30, 10}, {40, 40}, {20, 40}, {10, 20}, {20, 10}};

    graph::algorithms::Polygon test_polygon(std::move(points));
    
    Point test_point{20, 20};

    volatile int dummy = 0;

    for (size_t i = 0; i < test_count; i++) {
        int r = test_polygon.contains(test_point);
        dummy += r;
        // test_point = Point{rand() * 0.5, rand() * 0.5};
    }

    std::cout << dummy << "/" << test_count << std::endl;
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
    auto geom = reader.read(content);

    auto bbox = geom->getEnvelopeInternal();
    double min_lon = bbox->getMinX();
    double max_lon = bbox->getMaxX();
    double min_lat = bbox->getMinY();
    double max_lat = bbox->getMaxY();
    std::vector<double> lon_vec;
    std::vector<double> lat_vec;
    size_t lat_cnt = (max_lat - min_lat) / lat_step;
    size_t lon_cnt = (max_lon - min_lon) / lon_step;

    std::cout << lon_cnt << " " << lat_cnt << " " << lon_cnt * lat_cnt << std::endl;
    std::cout << min_lon << " " << max_lon << " " << min_lat << " " << max_lat << std::endl;

    std::vector<size_t> point_ids_vec(lon_cnt * lat_cnt);
    std::iota(point_ids_vec.begin(), point_ids_vec.end(), 0);

    for (double lon = min_lon; lon <= max_lon; lon += lon_step) {
        for (double lat = min_lat; lat <= max_lat; lat += lat_step) {
            lon_vec.push_back(lon);
            lat_vec.push_back(lat);
        }
    }

    auto filter_points_task = [&lon_vec, &lat_vec, &geom, &geometry_factory](size_t point_idx, int thread_num) {
        double lon = lon_vec[point_idx];
        double lat = lat_vec[point_idx];
        auto point = geometry_factory->createPoint(Coordinate(lon, lat));
        std::string res = "";

        if (geom->contains(point.get())) {
            res = geo_utils::geohash::encode(lon, lat);
        }

        return res;
    };
    std::cout << "Start threads" << std::endl;
    auto geohash_vec = graph::algorithms::run_in_threads(point_ids_vec, n_threads, filter_points_task);
}


// int k = ([]{}(),0)

TEST(test_geoms, test_geojson_reader) {
    using namespace geos::io;
    using namespace geos::geom;

    const std::string filename = "test/tests/test_geo_utils/resources/moscow_city_polygon.geojson";

    

    // Read file into string
    const std::string content = io::read_file(filename);

    // Parse GeoJSON string into GeoJSON objects

    GeoJSONReader reader;
    // GeoJSONFeatureCollection fc = reader.readFeatures(content);
    auto geom = reader.read(content);

    // Prepare WKT writer
    WKTWriter writer;

    std::cout << "geometry: " << writer.write(geom.get()) << std::endl;

    // Print out the features
    // for (auto& feature : fc) {

    //     // Read the geometry
    //     const Geometry* geom = feature.getGeometry();

    //     // Read the properties
    //     std::map<std::string, GeoJSONValue>& props = feature.getProperties();

    //     // Write all properties
    //     std::cout << "----------" << std::endl;
    //     for (const auto& prop : props) {
    //         std::cout << prop.first << ": " << prop.second << std::endl;
    //     }

    //     // Write WKT feometry
    //     std::cout << "geometry: " << writer.write(geom) << std::endl;
    // }
}
