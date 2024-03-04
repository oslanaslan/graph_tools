#include "geo_utils/osm_graph.h"
#include <gtest/gtest.h>
#include <graph.h>
#include <io/load_graphml.h>
#include <fstream>
#include <string>
#include <unordered_map>

const std::string TEST_GRAPH_FILENAME = "test/tests/test_io/resources/sahalin_region.graphml";
const float TOL = 0.01;

TEST(test_load_graphml, small_file) {
    osm::OSMNode test_start_node = osm::OSMNode{"2591428522"};
    std::unordered_map<osm::OSMNode, float> test_neighbors = {
        {osm::OSMNode{"2591428547"}, 185.386},
        {osm::OSMNode{"3128660940"}, 11017.845	},
        {osm::OSMNode{"2591428524"}, 31.215}
    };
    std::fstream f{TEST_GRAPH_FILENAME};
    auto graph = io::load_graphml(f);
    auto res_neighbors = graph.get_neighbors(test_start_node);

    for (auto& true_key_value : test_neighbors) {
        ASSERT_NEAR(res_neighbors.at(true_key_value.first), true_key_value.second, TOL);
    }

    for (auto& res_key_value : res_neighbors) {
        ASSERT_NEAR(test_neighbors.at(res_key_value.first), res_key_value.second, TOL);
    }
}