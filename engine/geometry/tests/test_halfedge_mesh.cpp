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
    EXPECT_TRUE(h01.is_valid());
    if (!h01.is_valid())
    {
        return;
    }
    EXPECT_EQ(mesh.interface.from_vertex(h01), fixture.v0);
    EXPECT_EQ(mesh.interface.to_vertex(h01), fixture.v1);
    EXPECT_EQ(mesh.interface.face(h01), fixture.f0);

    const auto h12 = mesh.interface.next_halfedge(h01);
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
    EXPECT_EQ(mesh.interface.next_halfedge(h20), h01);

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
