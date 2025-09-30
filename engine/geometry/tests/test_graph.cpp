#include <gtest/gtest.h>

#include "engine/geometry/graph/graph.hpp"

namespace geo = engine::geometry;
namespace graph_ns = engine::geometry::graph;

TEST(Graph, BuildsUndirectedConnectivity) {
    geo::Graph graph;

    const auto v0 = graph.interface.add_vertex({});
    const auto v1 = graph.interface.add_vertex({});
    const auto v2 = graph.interface.add_vertex({});

    const auto e01 = graph.interface.add_edge(v0, v1);
    EXPECT_TRUE(graph.interface.is_valid(e01));

    const auto e12 = graph.interface.add_edge(v1, v2);
    EXPECT_TRUE(graph.interface.is_valid(e12));

    const auto e20 = graph.interface.add_edge(v2, v0);
    EXPECT_TRUE(graph.interface.is_valid(e20));

    EXPECT_EQ(graph.interface.vertex_count(), 3U);
    EXPECT_EQ(graph.interface.edge_count(), 3U);

    EXPECT_TRUE(graph.interface.is_valid(v0));
    EXPECT_TRUE(graph.interface.is_valid(e01));

    EXPECT_EQ(graph.interface.valence(v0), 2U);
    EXPECT_EQ(graph.interface.valence(v1), 2U);
    EXPECT_EQ(graph.interface.valence(v2), 2U);

    auto incident = graph.interface.halfedges(v0);
    ASSERT_EQ(incident.size(), 2U);
    EXPECT_TRUE(
        (*incident.begin() == e01) ||
        (*incident.begin() == graph.interface.opposite_halfedge(e20))
    );

    auto neighbors = graph.interface.vertices(v0);
    ASSERT_EQ(neighbors.size(), 2U);
    EXPECT_TRUE(
        (*neighbors.begin() == v1 && *(++neighbors.begin()) == v2) ||
        (*neighbors.begin() == v2 && *(++neighbors.begin())== v1)
    );

    const auto start = graph.interface.from_vertex(e01);
    const auto end = graph.interface.to_vertex(e01);
    EXPECT_TRUE((start == v0 &&end == v1) || (start == v1 && end == v0));

    auto edge_property = graph.interface.add_edge_property<float>("e:length", 1.0F);
    auto e = graph.interface.edge(e01);
    EXPECT_FLOAT_EQ(edge_property[e], 1.0F);
    edge_property[e] = 2.5F;

    auto vertex_property = graph.interface.add_vertex_property<int>("v:valence", 0);
    vertex_property[v0] = static_cast<int>(graph.interface.valence(v0));

    const auto vertex_property_copy = graph.interface.get_vertex_property<int>("v:valence");
    EXPECT_EQ(vertex_property_copy[v0], 2);
}

TEST(Graph, CopiesAndClears) {
    geo::Graph graph;
    const auto v0 = graph.interface.add_vertex({});
    const auto v1 = graph.interface.add_vertex({});

    const auto e = graph.interface.add_edge(v0, v1);
    EXPECT_TRUE(graph.interface.is_valid(e));

    auto weights = graph.interface.add_vertex_property<float>("v:weight", 1.0F);
    weights[v0] = 3.5F;

    geo::Graph copy = graph;
    const auto weights_copy = copy.interface.get_vertex_property<float>("v:weight");
    EXPECT_FLOAT_EQ(weights_copy[v0], 3.5F);
    EXPECT_EQ(copy.interface.edge_count(), graph.interface.edge_count());

    graph.interface.clear();
    EXPECT_TRUE(graph.interface.is_empty());
    EXPECT_EQ(graph.interface.edge_count(), 0U);
    EXPECT_EQ(graph.interface.vertex_count(), 0U);

    const auto new_v = graph.interface.add_vertex({});
    EXPECT_TRUE(new_v.is_valid());
    auto new_weights = graph.interface.vertex_property<float>("v:weight", 0.0F);
    EXPECT_FLOAT_EQ(new_weights[new_v], 0.0F);
}
