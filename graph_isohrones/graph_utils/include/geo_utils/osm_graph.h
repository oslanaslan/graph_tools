#pragma once
#include <cstddef>
#include <functional>
#include <graph.h>
#include <library/const_string.h>
#include <string>

namespace osm {

struct OSMNode {
  lib::const_string node_id;
  float x;
  float y;

  bool operator==(const OSMNode &other) const {
    return node_id == other.node_id;
  }
};

using OSMGraph = graph::Graph<OSMNode>;

} // namespace osm

template <> struct std::hash<osm::OSMNode> {
  std::size_t operator()(const osm::OSMNode &node) const {
    return std::hash<lib::const_string>{}(node.node_id);
  }
};