#include <graph.h>
#include <cstddef>
#include <functional>
#include <string>

namespace osm {

struct OSMNode {
    std::string node_id;
    float x;
    float y;

    bool operator==(const OSMNode& other) const {
        return node_id == other.node_id;
    }
};



}

template<>
struct std::hash<osm::OSMNode> {
    std::size_t operator()(const osm::OSMNode& node) const {
        return std::hash<std::string>{}(node.node_id);
    }
};