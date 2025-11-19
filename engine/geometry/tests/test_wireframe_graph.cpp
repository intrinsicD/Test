#include <gtest/gtest.h>

#include "engine/geometry/mesh/wireframe_graph.hpp"
#include "engine/geometry/mesh/surface_mesh.hpp"

namespace
{
    using engine::geometry::SurfaceMesh;
    using engine::geometry::Graph;
}

TEST(WireframeGraph, BuildsEdgesFromTriangleMesh)
{
    SurfaceMesh mesh;
    mesh.positions = {
        {0.0F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
        {0.0F, 0.0F, 1.0F},
    };
    mesh.indices = {
        0U, 1U, 2U,
        0U, 2U, 3U,
    };

    const Graph graph = engine::geometry::mesh::build_wireframe_graph(mesh);
    const auto& interface = graph.interface;

    EXPECT_EQ(interface.vertex_count(), 4U);
    // Triangles should produce 5 unique edges
    EXPECT_EQ(interface.edge_count(), 5U);
}

TEST(WireframeGraph, IgnoresDegenerateIndices)
{
    SurfaceMesh mesh;
    mesh.positions = {
        {0.0F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F},
    };
    mesh.indices = {0U, 0U, 1U, 1U, 1U, 0U};

    const Graph graph = engine::geometry::mesh::build_wireframe_graph(mesh);
    const auto& interface = graph.interface;

    // Only one valid edge (0,1)
    EXPECT_EQ(interface.edge_count(), 1U);
}
