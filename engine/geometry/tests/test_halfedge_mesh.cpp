#include <gtest/gtest.h>

#include "engine/geometry/mesh/halfedge_mesh.hpp"

namespace geo = engine::geometry;

namespace
{
struct TriangleMeshFixture
{
    geo::Mesh mesh;
    geo::VertexHandle v0;
    geo::VertexHandle v1;
    geo::VertexHandle v2;
    geo::FaceHandle f0;
};

TriangleMeshFixture MakeTriangleMesh()
{
    TriangleMeshFixture fixture;

    const engine::math::vec3 p0{0.0F, 0.0F, 0.0F};
    const engine::math::vec3 p1{1.0F, 0.0F, 0.0F};
    const engine::math::vec3 p2{0.0F, 1.0F, 0.0F};

    fixture.v0 = fixture.mesh.interface.add_vertex(p0);
    fixture.v1 = fixture.mesh.interface.add_vertex(p1);
    fixture.v2 = fixture.mesh.interface.add_vertex(p2);

    const auto face = fixture.mesh.interface.add_triangle(fixture.v0, fixture.v1, fixture.v2);
    if (face)
    {
        fixture.f0 = *face;
    }

    return fixture;
}
} // namespace

TEST(HalfedgeMesh, AddTriangleBuildsConnectivity)
{
    auto fixture = MakeTriangleMesh();
    auto& mesh = fixture.mesh;

    EXPECT_TRUE(fixture.f0.is_valid());
    if (!fixture.f0.is_valid())
    {
        return;
    }

    EXPECT_EQ(mesh.interface.vertex_count(), 3U);
    EXPECT_EQ(mesh.interface.edge_count(), 3U);
    EXPECT_EQ(mesh.interface.halfedge_count(), 6U);
    EXPECT_EQ(mesh.interface.face_count(), 1U);

    EXPECT_TRUE(mesh.interface.is_boundary(fixture.v0));
    const auto boundary_halfedge = mesh.interface.halfedge(fixture.v0);
    EXPECT_TRUE(boundary_halfedge.is_valid());
    if (!boundary_halfedge.is_valid())
    {
        return;
    }
    EXPECT_TRUE(mesh.interface.is_boundary(boundary_halfedge));
    EXPECT_TRUE(mesh.interface.is_boundary(mesh.interface.edge(boundary_halfedge)));
    EXPECT_TRUE(mesh.interface.is_boundary(fixture.f0));

    EXPECT_EQ(mesh.interface.valence(fixture.v0), 2U);
    EXPECT_EQ(mesh.interface.valence(fixture.f0), 3U);

    const auto h01 = mesh.interface.find_halfedge(fixture.v0, fixture.v1);
    EXPECT_TRUE(h01.has_value());
    if (!h01.has_value())
    {
        return;
    }
    EXPECT_EQ(mesh.interface.from_vertex(h01.value()), fixture.v0);
    EXPECT_EQ(mesh.interface.to_vertex(h01.value()), fixture.v1);
    EXPECT_EQ(mesh.interface.face(h01.value()), fixture.f0);

    const auto h12 = mesh.interface.next_halfedge(h01.value());
    EXPECT_TRUE(h12.is_valid());
    if (!h12.is_valid())
    {
        return;
    }
    EXPECT_EQ(mesh.interface.from_vertex(h12), fixture.v1);
    EXPECT_EQ(mesh.interface.to_vertex(h12), fixture.v2);

    const auto h20 = mesh.interface.next_halfedge(h12);
    EXPECT_TRUE(h20.is_valid());
    if (!h20.is_valid())
    {
        return;
    }
    EXPECT_EQ(mesh.interface.from_vertex(h20), fixture.v2);
    EXPECT_EQ(mesh.interface.to_vertex(h20), fixture.v0);
    EXPECT_EQ(mesh.interface.next_halfedge(h20), h01.value());

    EXPECT_FLOAT_EQ(mesh.interface.position(fixture.v0)[0], 0.0F);
    EXPECT_FLOAT_EQ(mesh.interface.position(fixture.v1)[0], 1.0F);
    EXPECT_FLOAT_EQ(mesh.interface.position(fixture.v2)[1], 1.0F);

    EXPECT_TRUE(mesh.interface.is_triangle_mesh());
    EXPECT_TRUE(!mesh.interface.is_quad_mesh());
}

TEST(HalfedgeMesh, DeleteFaceMarksGarbageAndCollects)
{
    auto fixture = MakeTriangleMesh();
    auto& mesh = fixture.mesh;

    EXPECT_TRUE(fixture.f0.is_valid());
    if (!fixture.f0.is_valid())
    {
        return;
    }

    mesh.interface.delete_face(fixture.f0);

    EXPECT_EQ(mesh.interface.vertex_count(), 0U);
    EXPECT_EQ(mesh.interface.edge_count(), 0U);
    EXPECT_EQ(mesh.interface.halfedge_count(), 0U);
    EXPECT_EQ(mesh.interface.face_count(), 0U);

    EXPECT_EQ(mesh.interface.vertices_size(), 3U);
    EXPECT_EQ(mesh.interface.edges_size(), 3U);
    EXPECT_EQ(mesh.interface.halfedges_size(), 6U);
    EXPECT_EQ(mesh.interface.faces_size(), 1U);
    EXPECT_TRUE(mesh.interface.vertices_size() > mesh.interface.vertex_count());

    mesh.interface.garbage_collection();

    EXPECT_EQ(mesh.interface.vertices_size(), 0U);
    EXPECT_EQ(mesh.interface.edges_size(), 0U);
    EXPECT_EQ(mesh.interface.halfedges_size(), 0U);
    EXPECT_EQ(mesh.interface.faces_size(), 0U);
    EXPECT_TRUE(mesh.interface.vertices_size() == mesh.interface.vertex_count());
}

TEST(HalfedgeMesh, CopyIndependence)
{
    auto fixture = MakeTriangleMesh();
    auto& original = fixture.mesh;

    ASSERT_TRUE(fixture.f0.is_valid());
    auto area = original.interface.face_property<float>("f:copy_area", 0.0F);
    area[fixture.f0] = 0.5F;

    geo::Mesh copy(original);
    auto copy_area = copy.interface.get_face_property<float>("f:copy_area");

    copy_area[fixture.f0] = 1.25F;
    copy.interface.position(fixture.v0)[0] = -2.0F;
    const auto v3 = copy.interface.add_vertex({0.0F, 0.0F, 1.0F});

    EXPECT_FLOAT_EQ(area[fixture.f0], 0.5F);
    EXPECT_FLOAT_EQ(copy_area[fixture.f0], 1.25F);
    EXPECT_FLOAT_EQ(original.interface.position(fixture.v0)[0], 0.0F);
    EXPECT_FLOAT_EQ(copy.interface.position(fixture.v0)[0], -2.0F);
    EXPECT_TRUE(v3.is_valid());
    EXPECT_EQ(original.interface.vertex_count(), 3U);
    EXPECT_EQ(copy.interface.vertex_count(), 4U);
    EXPECT_EQ(original.interface.face_count(), 1U);
    EXPECT_EQ(copy.interface.face_count(), 1U);

    geo::Mesh assigned;
    assigned = original;
    auto assigned_area = assigned.interface.get_face_property<float>("f:copy_area");
    assigned_area[fixture.f0] = 2.0F;

    EXPECT_FLOAT_EQ(area[fixture.f0], 0.5F);
    EXPECT_FLOAT_EQ(assigned_area[fixture.f0], 2.0F);
    EXPECT_EQ(assigned.interface.face_count(), original.interface.face_count());
    EXPECT_EQ(assigned.interface.vertex_count(), original.interface.vertex_count());
}
