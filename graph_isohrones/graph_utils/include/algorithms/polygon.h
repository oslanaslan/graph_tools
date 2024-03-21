#pragma once

#include <algorithm>
#include <limits>
#include <vector>

namespace graph {
namespace algorithms {

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

struct Polygon {
  std::vector<Point2d> points;
  BoundingBox bbox;

  Polygon(std::vector<Point2d> verices) : points(std::move(verices)) {
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

  int direction(Point2d pi, Point2d pj, Point2d pk) {
    return (pk.x - pi.x) * (pj.y - pi.y) - (pj.x - pi.x) * (pk.y - pi.y);
  }

  bool onSegment(Point2d pi, Point2d pj, Point2d pk) {
    if (std::min(pi.x, pj.x) <= pk.x && pk.x <= std::max(pi.x, pj.x) &&
        std::min(pi.y, pj.y) <= pk.y && pk.y <= std::max(pi.y, pj.y)) {
      return true;
    } else {
      return false;
    }
  }

  bool segmentIntersect(Point2d p1, Point2d p2, Point2d p3, Point2d p4) {
    int d1 = direction(p3, p4, p1);
    int d2 = direction(p3, p4, p2);
    int d3 = direction(p1, p2, p3);
    int d4 = direction(p1, p2, p4);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
      return true;
    } else if (d1 == 0 && onSegment(p3, p4, p1)) {
      return true;
    } else if (d2 == 0 && onSegment(p3, p4, p2)) {
      return true;
    } else if (d3 == 0 && onSegment(p1, p2, p3)) {
      return true;
    } else if (d4 == 0 && onSegment(p1, p2, p4)) {
      return true;
    } else {
      return false;
    }
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

} // namespace algorithms
} // namespace graph