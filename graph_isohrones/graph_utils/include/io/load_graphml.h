#pragma once

#include <geo_utils/osm_graph.h>
#include <graphmlpp.hpp>


namespace io {

inline osm::OSMGraph load_graphml(std::istream &ifs) {
  std::unordered_map<osm::OSMNode, std::unordered_map<osm::OSMNode, float>>
      edges;
  std::unordered_map<nonstd::string_view, osm::OSMNode> nodes;

  gmlpp::Loader loader;
  loader.ReadStream(ifs);

  loader.Visit(
      [&nodes](const gmlpp::Loader::ElementView &node) {
        // TODO: parse node to OSMNode
        osm::OSMNode osm_node{node["id"].data(), node.Extract<float>("x"), node.Extract<float>("y")};
        nodes.insert({node["id"], osm_node});
      },
      [&edges, &nodes](const gmlpp::Loader::ElementView &edge) {
        // TODO: add edge to graph
        nonstd::string_view start_id = edge["source"];
        nonstd::string_view end_id = edge["target"];
        float length = edge.Extract<float>("length");

        edges[nodes.at(start_id)][nodes.at(end_id)] = length;
      });
  return osm::OSMGraph(std::move(edges));
}

} // namespace io