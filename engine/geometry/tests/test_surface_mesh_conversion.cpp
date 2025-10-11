#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"

namespace geo = engine::geometry;

TEST(SurfaceMeshConversion, RoundTripPreservesTopology)
{
    const geo::SurfaceMesh original = geo::make_unit_quad();

    geo::Mesh halfedge_container;
    ASSERT_NO_THROW(geo::mesh::build_halfedge_from_surface_mesh(original, halfedge_container.interface));

    const auto rebuilt = geo::mesh::build_surface_mesh_from_halfedge(halfedge_container.interface);

    EXPECT_EQ(rebuilt.positions.size(), original.positions.size());
    EXPECT_EQ(rebuilt.rest_positions.size(), original.rest_positions.size());
    EXPECT_EQ(rebuilt.indices.size(), original.indices.size());

    for (std::size_t i = 0; i < original.positions.size(); ++i)
    {
        EXPECT_FLOAT_EQ(rebuilt.positions[i][0], original.positions[i][0]);
        EXPECT_FLOAT_EQ(rebuilt.positions[i][1], original.positions[i][1]);
        EXPECT_FLOAT_EQ(rebuilt.positions[i][2], original.positions[i][2]);
    }

    for (std::size_t i = 0; i < original.indices.size(); ++i)
    {
        EXPECT_EQ(rebuilt.indices[i], original.indices[i]);
    }

    EXPECT_FALSE(rebuilt.normals.empty());
    EXPECT_FLOAT_EQ(rebuilt.normals.front()[1], 1.0F);
}

TEST(SurfaceMeshConversion, RejectsMalformedIndices)
{
    geo::SurfaceMesh surface;
    surface.positions = {engine::math::vec3{0.0F, 0.0F, 0.0F},
                         engine::math::vec3{1.0F, 0.0F, 0.0F},
                         engine::math::vec3{0.0F, 1.0F, 0.0F}};
    surface.indices = {0U, 1U};

    geo::Mesh container;
    EXPECT_THROW(geo::mesh::build_halfedge_from_surface_mesh(surface, container.interface), std::runtime_error);
}

TEST(SurfaceMeshConversion, RejectsDegenerateTriangles)
{
    geo::SurfaceMesh surface;
    surface.positions = {engine::math::vec3{0.0F, 0.0F, 0.0F},
                         engine::math::vec3{1.0F, 0.0F, 0.0F},
                         engine::math::vec3{0.0F, 1.0F, 0.0F}};
    surface.indices = {0U, 0U, 1U};

    geo::Mesh container;
    EXPECT_THROW(geo::mesh::build_halfedge_from_surface_mesh(surface, container.interface), std::runtime_error);
}

TEST(SurfaceMeshConversion, ThrowsWhenHalfedgeReferencesDeletedVertex)
{
    geo::Mesh container;
    const auto v0 = container.interface.add_vertex({0.0F, 0.0F, 0.0F});
    const auto v1 = container.interface.add_vertex({1.0F, 0.0F, 0.0F});
    const auto v2 = container.interface.add_vertex({0.0F, 1.0F, 0.0F});

    const auto face = container.interface.add_triangle(v0, v1, v2);
    ASSERT_TRUE(face.has_value());

    container.interface.delete_vertex(v1);

    EXPECT_THROW(static_cast<void>(geo::mesh::build_surface_mesh_from_halfedge(container.interface)), std::runtime_error);
}

