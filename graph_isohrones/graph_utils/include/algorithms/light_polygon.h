#pragma once

#include "geos/geom/LinearRing.h"
#include "geos/geom/Point.h"
#include "geos/geom/Polygon.h"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
#include <geos/geom/Geometry.h>

namespace graph {
namespace algorithms {

using namespace geos::geom;
using namespace geos::io;

struct Point2d {
  double x;
  double y;
};

struct BoundingBox {
  double xmin;
  double xmax;
  double ymin;
  double ymax;

  BoundingBox(double xmin, double xmax, double ymin, double ymax) {
    this->xmin = xmin;
    this->xmax = xmax;
    this->ymin = ymin;
    this->ymax = ymax;
  }

  BoundingBox() {
    this->xmin = std::numeric_limits<double>::max();
    this->xmax = std::numeric_limits<double>::min();
    this->ymin = std::numeric_limits<double>::max();
    this->ymax = std::numeric_limits<double>::min();
  }
};

struct LightSimplePolygon {
  std::vector<Point2d> points;
  BoundingBox bbox;

  LightSimplePolygon() = default;

  LightSimplePolygon(std::vector<Point2d> verices) : points(std::move(verices)) {
    calcBoundingBox();
  }

  LightSimplePolygon(const LinearRing* geos_linear_ring) {
    if (!geos_linear_ring->isRing() || !geos_linear_ring->isClosed()) {
      std::runtime_error("geos_linear_ring must be closed LinearRing");
    }

    std::vector<CoordinateXY> coords_vec;
    geos_linear_ring->getCoordinates()->toVector(coords_vec);
    this->points.resize(coords_vec.size());

    for (size_t i = 0; i < coords_vec.size(); i++) {
      auto geos_point = coords_vec[i];
      this->points[i] = Point2d{geos_point.x, geos_point.y};
    }
    calcBoundingBox();
  }

  bool inBoundingBox(Point2d point) {
    if (point.x < bbox.xmin || point.x > bbox.xmax || point.y < bbox.ymin ||
        point.y > bbox.ymax) {
      return false;
    } else {
      return true;
    }
  }

  bool contains(double lon, double lat) {
    return this->contains(Point2d{lon, lat});
  }

  bool contains(Point2d point) {
    if (!inBoundingBox(point)) {
      return false;
    }

    // create a ray (segment) starting from the given point,
    // and to the point outside of polygon.
    Point2d outside{bbox.xmin - 1, bbox.ymin};
    int intersections = 0;
    // check intersections between the ray and every side of the polygon.
    for (int i = 0; i < points.size() - 1; ++i) {
      if (segmentIntersect(point, outside, points[i], points[i + 1])) {
        intersections++;
      }
    }
    // check the last line
    if (segmentIntersect(point, outside, points[points.size() - 1],
                         points[0])) {
      intersections++;
    }
    return (intersections % 2 != 0);
  }

  double direction(Point2d pi, Point2d pj, Point2d pk) {
    return (pk.x - pi.x) * (pj.y - pi.y) - (pj.x - pi.x) * (pk.y - pi.y);
  }

  // bool onSegment(Point2d pi, Point2d pj, Point2d pk) {
  //   if (std::min(pi.x, pj.x) <= pk.x && pk.x <= std::max(pi.x, pj.x) &&
  //       std::min(pi.y, pj.y) <= pk.y && pk.y <= std::max(pi.y, pj.y)) {
  //     return true;
  //   } else {
  //     return false;
  //   }
  // }

  bool segmentIntersect(Point2d p1, Point2d p2, Point2d p3, Point2d p4) {
    auto d1 = direction(p3, p4, p1);
    auto d2 = direction(p3, p4, p2);
    auto d3 = direction(p1, p2, p3);
    auto d4 = direction(p1, p2, p4);

    // if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
    //     ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
    //   return true;
    // } else if (d1 == 0 && onSegment(p3, p4, p1)) {
    //   return true;
    // } else if (d2 == 0 && onSegment(p3, p4, p2)) {
    //   return true;
    // } else if (d3 == 0 && onSegment(p1, p2, p3)) {
    //   return true;
    // } else if (d4 == 0 && onSegment(p1, p2, p4)) {
    //   return true;
    // } else {
    //   return false;
    // }

    return (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)));
  }

private:
  void calcBoundingBox() {
    for (const auto &point : points) {
      if (point.x < bbox.xmin) {
        bbox.xmin = point.x;
      } else if (point.x > bbox.xmax) {
        bbox.xmax = point.x;
      }
      if (point.y < bbox.ymin) {
        bbox.ymin = point.y;
      } else if (point.y > bbox.ymax) {
        bbox.ymax = point.y;
      }
    }
  }
};

struct LightPolygon {
  LightSimplePolygon external_polygon;
  std::vector<LightSimplePolygon> internal_polygons_vec;

  LightPolygon() = default;

  LightPolygon(const Polygon* geos_polygon) {
    if (!geos_polygon->isPolygonal() || geos_polygon->getGeometryType() != "Polygon") {
      std::runtime_error("Passed geos_polygon must be type of 'Polygon'");
    }

    size_t holes_count = geos_polygon->getNumInteriorRing();
    std::vector<CoordinateXY> coords_vec;
    auto exterior_ring = geos_polygon->getExteriorRing();
    this->external_polygon = LightSimplePolygon(exterior_ring);

    for (size_t hole_i = 0; hole_i < holes_count; hole_i++) {
      auto internal_ring = geos_polygon->getInteriorRingN(hole_i);
      this->internal_polygons_vec.push_back(LightSimplePolygon(internal_ring));
    }
  }

  bool contains(double lon, double lat) {
    return this->contains(Point2d{lon, lat});
  }

  bool contains(Point2d point) {
    for (auto hole : internal_polygons_vec) {
      if (hole.contains(point)) {
        return false;
      }
    }

    return external_polygon.contains(point);
  }
};

struct LightMultiPolygon {
  std::vector<LightPolygon> polygons_vec;

  LightMultiPolygon() = default;

  LightMultiPolygon(const Geometry* geos_geometry) {
    if (!geos_geometry->isPolygonal()) {
      std::runtime_error("Passed geos_geometry must be type of 'Polygon' or 'MultiPolygon'");
    }

    size_t polygons_count = geos_geometry->getNumGeometries();
    polygons_vec.resize(polygons_count);

    for (size_t poly_i = 0; poly_i < polygons_count; poly_i++) {
      const Polygon* geos_polygon = (const Polygon*)geos_geometry->getGeometryN(poly_i);
      polygons_vec[poly_i] = LightPolygon(geos_polygon);
    }
  }

  bool contains(double lon, double lat) {
    return this->contains(Point2d{lon, lat});
  }

  bool contains(Point2d point) {
    for (auto polygon : polygons_vec) {
      if (polygon.contains(point)) {
        return true;
      }
    }

    return false;
  }
};

} // namespace algorithms
} // namespace graph