#include "geo_utils/osm_graph.h"
#include <gtest/gtest.h>
#include <graph.h>
#include <io/load_graphml.h>
#include <fstream>
#include <string>
#include <unordered_map>

const std::string TEST_GRAPH_FILENAME = "sahalin_region.graphml";

TEST(test_load_graphml, small_file) {
    osm::OSMNode test_start_node = osm::OSMNode{"2591428522"};
    std::unordered_map<osm::OSMNode, float> test_neighbors = {
        {osm::OSMNode{"2591428547"}, 185.386},
        {osm::OSMNode{"3128660940"}, 11017.845	},
        {osm::OSMNode{"2591428524"}, 31.215}
    };
    std::fstream f{TEST_GRAPH_FILENAME};
    auto graph = io::load_graphml(f);

    ASSERT_EQ(graph.get_neighbors(test_start_node), test_neighbors);
}