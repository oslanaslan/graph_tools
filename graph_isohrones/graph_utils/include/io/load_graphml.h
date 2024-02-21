#pragma once

#include <geo_utils/osm_graph.h>
#include <graphmlpp.hpp>


namespace io {

osm::OSMGraph load_graphml(std::istream &ifs) {
  std::unordered_map<osm::OSMNode, std::unordered_map<osm::OSMNode, float>>
      edges;

  gmlpp::Loader loader;
  loader.ReadStream(ifs);

  loader.Visit(
      [](const gmlpp::Loader::ElementView &node) {
        // TODO: parse node to OSMNode
      },
      [](const gmlpp::Loader::ElementView &edge) {
        // TODO: add edge to graph
      });
  return osm::OSMGraph(std::move(edges));
}

} // namespace io