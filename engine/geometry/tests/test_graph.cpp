#include <gtest/gtest.h>

#include "engine/geometry/graph/graph.hpp"

namespace geo = engine::geometry;
namespace graph_ns = engine::geometry::graph;

TEST(Graph, BuildsUndirectedConnectivity)
{
    geo::Graph graph;

    const auto v0 = graph.add_vertex();
    const auto v1 = graph.add_vertex();
    const auto v2 = graph.add_vertex();

    const auto e01 = graph.add_edge(v0, v1);
    EXPECT_TRUE(e01.has_value());
    if (!e01.has_value())
    {
        return;
    }

    const auto e12 = graph.add_edge(v1, v2);
    EXPECT_TRUE(e12.has_value());
    if (!e12.has_value())
    {
        return;
    }

    const auto e20 = graph.add_edge(v2, v0);
    EXPECT_TRUE(e20.has_value());
    if (!e20.has_value())
    {
        return;
    }

    EXPECT_EQ(graph.vertex_count(), 3U);
    EXPECT_EQ(graph.edge_count(), 3U);

    EXPECT_TRUE(graph.is_valid(v0));
    EXPECT_TRUE(graph.is_valid(*e01));

    EXPECT_EQ(graph.degree(v0), 2U);
    EXPECT_EQ(graph.degree(v1), 2U);
    EXPECT_EQ(graph.degree(v2), 2U);

    const auto& incident = graph.incident_edges(v0);
    ASSERT_EQ(incident.size(), 2U);
    EXPECT_TRUE((incident[0] == *e01) || (incident[0] == *e20));

    const auto neighbors = graph.neighbors(v0);
    ASSERT_EQ(neighbors.size(), 2U);
    EXPECT_TRUE((neighbors[0] == v1 && neighbors[1] == v2) || (neighbors[0] == v2 && neighbors[1] == v1));

    const auto endpoints = graph.endpoints(*e01);
    EXPECT_TRUE((endpoints.first == v0 && endpoints.second == v1) || (endpoints.first == v1 && endpoints.second == v0));

    auto edge_property = graph.add_edge_property<float>("e:length", 1.0F);
    EXPECT_FLOAT_EQ(edge_property[*e01], 1.0F);
    edge_property[*e01] = 2.5F;

    auto vertex_property = graph.add_vertex_property<int>("v:valence", 0);
    vertex_property[v0] = static_cast<int>(graph.degree(v0));

    const auto vertex_property_copy = graph.get_vertex_property<int>("v:valence");
    EXPECT_EQ(vertex_property_copy[v0], 2);

    const auto invalid_edge = graph.add_edge(graph_ns::VertexHandle(42U), v1);
    EXPECT_TRUE(!invalid_edge.has_value());
}

TEST(Graph, CopiesAndClears)
{
    geo::Graph graph;
    const auto v0 = graph.add_vertex();
    const auto v1 = graph.add_vertex();

    const auto e = graph.add_edge(v0, v1);
    EXPECT_TRUE(e.has_value());
    if (!e.has_value())
    {
        return;
    }

    auto weights = graph.add_vertex_property<float>("v:weight", 1.0F);
    weights[v0] = 3.5F;

    geo::Graph copy = graph;
    const auto weights_copy = copy.get_vertex_property<float>("v:weight");
    EXPECT_FLOAT_EQ(weights_copy[v0], 3.5F);
    EXPECT_EQ(copy.edge_count(), graph.edge_count());

    graph.clear();
    EXPECT_TRUE(graph.is_empty());
    EXPECT_EQ(graph.edge_count(), 0U);
    EXPECT_EQ(graph.vertex_count(), 0U);

    const auto new_v = graph.add_vertex();
    EXPECT_TRUE(new_v.is_valid());
    auto new_weights = graph.vertex_property<float>("v:weight", 0.0F);
    EXPECT_FLOAT_EQ(new_weights[new_v], 0.0F);
}

