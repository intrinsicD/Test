#include "engine/geometry/topology/surface_topology.hpp"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"
#include "engine/math/common.hpp"

namespace
{
    engine::geometry::SurfaceMesh MakeMesh(std::vector<engine::math::vec3> positions,
                                           std::vector<std::uint32_t> indices)
    {
        engine::geometry::SurfaceMesh mesh{};
        mesh.positions = std::move(positions);
        mesh.indices = std::move(indices);
        return mesh;
    }
}

TEST(SurfaceTopology, DetectsBoundaryEdges)
{
    auto mesh = MakeMesh({
                             {0.0F, 0.0F, 0.0F},
                             {1.0F, 0.0F, 0.0F},
                             {1.0F, 1.0F, 0.0F},
                             {0.0F, 1.0F, 0.0F},
                         },
                         {
                             0, 1, 2,
                             0, 2, 3,
                         });

    const auto summary = engine::geometry::AnalyzeSurfaceTopology(mesh, engine::math::radians(30.0F));

    EXPECT_EQ(summary.vertices.size(), 4U);
    EXPECT_EQ(summary.edges.size(), 5U);

    const auto boundary_edges = std::count_if(
        summary.edges.begin(), summary.edges.end(), [](const engine::geometry::SurfaceEdgeTag& edge)
        {
            return edge.is_boundary;
        });
    EXPECT_EQ(boundary_edges, 4);

    for (const auto& vertex : summary.vertices)
    {
        EXPECT_TRUE(vertex.is_boundary);
        EXPECT_TRUE(vertex.is_feature);
    }
}

TEST(SurfaceTopology, DetectsCreaseEdges)
{
    auto mesh = MakeMesh({
                             {0.0F, 0.0F, 0.0F},
                             {1.0F, 0.0F, 0.0F},
                             {0.0F, 1.0F, 0.0F},
                             {0.0F, 0.0F, 1.0F},
                         },
                         {
                             0, 1, 2,
                             0, 1, 3,
                             0, 2, 3,
                             1, 2, 3,
                         });

    const auto summary = engine::geometry::AnalyzeSurfaceTopology(mesh, engine::math::radians(30.0F));

    EXPECT_EQ(summary.edges.size(), 6U);

    for (const auto& edge : summary.edges)
    {
        EXPECT_FALSE(edge.is_boundary);
        EXPECT_TRUE(edge.is_crease);
        EXPECT_GT(edge.dihedral_angle, 0.0F);
    }

    for (const auto& vertex : summary.vertices)
    {
        EXPECT_FALSE(vertex.is_boundary);
        EXPECT_TRUE(vertex.is_feature);
    }
}

TEST(SurfaceTopology, FlagsNonManifoldEdges)
{
    auto mesh = MakeMesh({
                             {0.0F, 0.0F, 0.0F},
                             {1.0F, 0.0F, 0.0F},
                             {0.0F, 1.0F, 0.0F},
                             {0.0F, 0.0F, 1.0F},
                             {0.0F, 0.0F, -1.0F},
                         },
                         {
                             0, 1, 2,
                             0, 1, 3,
                             0, 1, 4,
                         });

    const auto summary = engine::geometry::AnalyzeSurfaceTopology(mesh, engine::math::radians(45.0F));

    const auto non_manifold_edge = std::find_if(
        summary.edges.begin(), summary.edges.end(), [](const engine::geometry::SurfaceEdgeTag& edge)
        {
            return edge.is_non_manifold;
        });
    ASSERT_NE(non_manifold_edge, summary.edges.end());
    EXPECT_FALSE(non_manifold_edge->is_boundary);
    EXPECT_EQ(std::min(non_manifold_edge->v0, non_manifold_edge->v1), 0U);
    EXPECT_EQ(std::max(non_manifold_edge->v0, non_manifold_edge->v1), 1U);

    EXPECT_TRUE(summary.vertices[0].is_feature);
    EXPECT_TRUE(summary.vertices[1].is_feature);
}