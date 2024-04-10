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

namespace graph {
namespace algorithms {

std::vector<std::string> polygon_geohashing(const Geometry* geos_polygon, int n_threads) {
    constexpr double lon_step = 0.0016 / 2;
    constexpr double lat_step = 0.0009 / 2;
    GeometryFactory::Ptr geometry_factory = GeometryFactory::create();
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

} // namespace algorithms
} // namespace graph