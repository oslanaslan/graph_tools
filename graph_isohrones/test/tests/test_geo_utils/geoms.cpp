#include "algorithms/parallel.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/geom/Point.h"
#include "geos/geom/Polygon.h"
#include <__chrono/duration.h>
#include <chrono>
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
#include <algorithms/light_polygon.h>

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
    size_t test_count = 10;
    // Two polygons
    // std::string test_geometry_wkt = "MULTIPOLYGON (((30 20, 45 40, 10 40, 30 20)),((15 5, 40 10, 10 20, 5 10, 15 5)))";
    // Two polygons, one with hole
    std::string test_geometry_wkt = "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)),((20 35, 10 30, 10 10, 30 5, 45 20, 20 35),(30 20, 20 15, 20 25, 30 20)))";
    // Simple polygon
    // std::string test_geometry_wkt = "POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))";

    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();
    WKTReader wkt_reader(*geometry_factory);
    WKTWriter wkt_writer;
    std::unique_ptr<Geometry> test_polygon(wkt_reader.read(test_geometry_wkt));
    std::unique_ptr<Point> test_point = geometry_factory->createPoint(Coordinate(20, 20));

    // std::cout << wkt_writer.write(test_polygon.get()) << std::endl;
    // std::cout << wkt_writer.write(test_point.get()) << std::endl;
    // std::cout << test_polygon->contains(test_point.get()) << std::endl;

    // std::cout << "Allocs: " << allocations << std::endl;

    // test_polygon->contains(test_point.get());
    // volatile int dummy = 0;

    int geoms_count = test_polygon->getNumGeometries();
    std::cout << test_polygon->getGeometryType() << std::endl;
    std::cout << geoms_count << std::endl;
    std::cout << "Is Polygon " << test_polygon->isPolygonal() << std::endl;

    for (size_t geom_i = 0; geom_i < geoms_count; geom_i++) {
        auto cur_geom = (Polygon *)test_polygon->getGeometryN(geom_i);
        size_t holes_count = cur_geom->getNumInteriorRing();
        
        std::cout << cur_geom->getGeometryType() << std::endl;
        std::cout << "Int ring count: " << holes_count << std::endl;
        std::cout << "Is Polygon cur_geom " << cur_geom->isPolygonal() << std::endl;

        // auto coords_seq = cur_geom->getExteriorRing()->getCoordinates();
        std::vector<CoordinateXY> coords_vec;
        auto line_ring = cur_geom->getExteriorRing();
        line_ring->getCoordinates()->toVector(coords_vec);
        std::cout << coords_vec[0].x << std::endl;
        
        for (size_t hole_i = 0; hole_i < holes_count; hole_i++) {
            std::vector<CoordinateXY> hole_coords_vec;
            auto internal_ring = cur_geom->getInteriorRingN(hole_i);
            std::cout << "Is Polygon line ring " << internal_ring->isPolygonal() << std::endl;

        }
    }

    // std::cout << test_polygon->getBoundary()->getNumGeometries() << std::endl;
    // auto internal_point = test_polygon->getInteriorPoint();
    // std::cout << test_polygon->contains(internal_point.get()) << std::endl;

    // std::cout << test_polygon->getNumInteriorRing() << std::endl;

    // for (size_t i = 0; i < test_count; i++) {
    //     // size_t old = alloc;
    //     int r = test_polygon->contains(test_point.get());
    //     dummy = dummy + r;
    //     // alloc = old;
    // }

    // EXPECT_NE(dummy, 0);

    // std::cout << "Allocs: " << allocations << std::endl;
}

TEST(test_geoms, test_light_multipolygon) {
    using namespace geos::io;
    using namespace geos::geom;

    std::string test_geometry_wkt = "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)),((20 35, 10 30, 10 10, 30 5, 45 20, 20 35),(30 20, 20 15, 20 25, 30 20)))";
    std::vector<graph::algorithms::Point2d> test_points_vec = {
        {25, 20}, // in hole
        {35, 20}, // in bigger polygon
        {35, 40}, // in smaller polygon
        {20, 40} // outside
    };
    std::vector<bool> true_res_vec = {false, true, true, false};
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();
    WKTReader wkt_reader(*geometry_factory);
    WKTWriter wkt_writer;
    std::unique_ptr<Geometry> test_polygon(wkt_reader.read(test_geometry_wkt));

    // Test GEOS Geometry to LightPolygon conversion
    graph::algorithms::LightMultiPolygon light_multipolygon(test_polygon.get());
    
    for (size_t i = 0; i < test_points_vec.size(); i++) {
        auto test_point = test_points_vec[i];
        bool true_res = true_res_vec[i];
        bool res = light_multipolygon.contains(test_point);

        EXPECT_EQ(true_res, res);
    }
}

TEST(test_geoms, test_benchmark_light_multipolygon) {
    using namespace geos::io;
    using namespace geos::geom;
    using LightMultiPolygon = graph::algorithms::LightMultiPolygon;
    using LightPolygon = graph::algorithms::LightPolygon;
    using Point2d = graph::algorithms::Point2d;

    const std::string filename = "test/tests/test_geo_utils/resources/dolgoprudniy_city_polygon.geojson";
    constexpr double lon_step = 0.0016 / 2;
    constexpr double lat_step = 0.0009 / 2;
    size_t test_run_counts = 100'000;

    GeoJSONReader reader;
    WKTWriter writer;
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();

    // Read file into string and parse geojson
    std::string content = io::read_file(filename);
    auto geom_collection = reader.read(content);
    auto geos_polygon = dynamic_cast<const Polygon*>(geom_collection->getGeometryN(0));
    LightPolygon light_polygon(geos_polygon);
    
    volatile size_t dummy = 0;

    // LightPolygon
    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < test_run_counts; i++) {
        Point2d test_point{37.514230, 55.933302};
        auto res = light_polygon.contains(test_point);
        dummy += res;
    }

    auto light_polygon_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
    std::cout << "LightPolygon: " << light_polygon_time << std::endl;

    // Geos Polygon
    start_time = std::chrono::steady_clock::now();


    for (size_t i = 0; i < test_run_counts; i++) {
        auto test_geos_point = geometry_factory->createPoint(Coordinate(37.514230, 55.933302));
        auto res = geos_polygon->contains(test_geos_point.get());
        dummy += res;
    }

    auto geos_polygon_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    std::cout << "GEOS Polygon: " << geos_polygon_time << std::endl;

    // Geos GeometryCollection
    start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < test_run_counts; i++) {
        auto test_geos_point = geometry_factory->createPoint(Coordinate(37.514230, 55.933302));
        auto res = geom_collection->contains(test_geos_point.get());
        dummy += res;
    }

    auto geos_geometry_collection_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    std::cout << "GEOS GeometryCollection: " << geos_geometry_collection_time << std::endl;
}

TEST(test_geoms, test_custom_polygons) {
    using LightPoint = graph::algorithms::Point2d;

    // size_t test_count = 1'000'000;
    constexpr size_t test_count = 1'000'000;

    std::string test_polygon_wkt = "POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))";

    std::vector<LightPoint> points{{30, 10}, {40, 40}, {20, 40}, {10, 20}};

    graph::algorithms::LightSimplePolygon test_polygon(std::move(points));
    
    LightPoint test_point{20, 20};

    // std::cout << test_polygon.points() << std::endl();

    volatile int dummy = 0;

    for (size_t i = 0; i < test_count; i++) {
        int r = test_polygon.contains(test_point);
        dummy = dummy + r;
        // test_point = Point{rand() * 0.5, rand() * 0.5};
    }

    std::cout << dummy << "/" << test_count << std::endl;
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
