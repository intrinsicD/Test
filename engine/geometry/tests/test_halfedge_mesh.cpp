#include <gtest/gtest.h>

#include "engine/geometry/mesh/halfedge_mesh.hpp"

namespace geo = engine::geometry;

namespace
{
struct TriangleMeshFixture
{
    geo::HalfedgeMesh mesh;
    geo::VertexHandle v0;
    geo::VertexHandle v1;
    geo::VertexHandle v2;
    geo::FaceHandle f0;
};

TriangleMeshFixture MakeTriangleMesh()
{
    TriangleMeshFixture fixture;

    const geo::Point3 p0{0.0F, 0.0F, 0.0F};
    const geo::Point3 p1{1.0F, 0.0F, 0.0F};
    const geo::Point3 p2{0.0F, 1.0F, 0.0F};

    fixture.v0 = fixture.mesh.add_vertex(p0);
    fixture.v1 = fixture.mesh.add_vertex(p1);
    fixture.v2 = fixture.mesh.add_vertex(p2);

    const auto face = fixture.mesh.add_triangle(fixture.v0, fixture.v1, fixture.v2);
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

    EXPECT_EQ(mesh.vertex_count(), 3U);
    EXPECT_EQ(mesh.edge_count(), 3U);
    EXPECT_EQ(mesh.halfedge_count(), 6U);
    EXPECT_EQ(mesh.face_count(), 1U);

    EXPECT_TRUE(mesh.is_boundary(fixture.v0));
    const auto boundary_halfedge = mesh.halfedge(fixture.v0);
    EXPECT_TRUE(boundary_halfedge.is_valid());
    if (!boundary_halfedge.is_valid())
    {
        return;
    }
    EXPECT_TRUE(mesh.is_boundary(boundary_halfedge));
    EXPECT_TRUE(mesh.is_boundary(mesh.edge(boundary_halfedge)));
    EXPECT_TRUE(mesh.is_boundary(fixture.f0));

    EXPECT_EQ(mesh.valence(fixture.v0), 2U);
    EXPECT_EQ(mesh.valence(fixture.f0), 3U);

    const auto h01 = mesh.find_halfedge(fixture.v0, fixture.v1);
    EXPECT_TRUE(h01.is_valid());
    if (!h01.is_valid())
    {
        return;
    }
    EXPECT_EQ(mesh.from_vertex(h01), fixture.v0);
    EXPECT_EQ(mesh.to_vertex(h01), fixture.v1);
    EXPECT_EQ(mesh.face(h01), fixture.f0);

    const auto h12 = mesh.next_halfedge(h01);
    EXPECT_TRUE(h12.is_valid());
    if (!h12.is_valid())
    {
        return;
    }
    EXPECT_EQ(mesh.from_vertex(h12), fixture.v1);
    EXPECT_EQ(mesh.to_vertex(h12), fixture.v2);

    const auto h20 = mesh.next_halfedge(h12);
    EXPECT_TRUE(h20.is_valid());
    if (!h20.is_valid())
    {
        return;
    }
    EXPECT_EQ(mesh.from_vertex(h20), fixture.v2);
    EXPECT_EQ(mesh.to_vertex(h20), fixture.v0);
    EXPECT_EQ(mesh.next_halfedge(h20), h01);

    EXPECT_FLOAT_EQ(mesh.position(fixture.v0)[0], 0.0F);
    EXPECT_FLOAT_EQ(mesh.position(fixture.v1)[0], 1.0F);
    EXPECT_FLOAT_EQ(mesh.position(fixture.v2)[1], 1.0F);

    EXPECT_TRUE(mesh.is_triangle_mesh());
    EXPECT_TRUE(!mesh.is_quad_mesh());
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

    mesh.delete_face(fixture.f0);

    EXPECT_EQ(mesh.vertex_count(), 0U);
    EXPECT_EQ(mesh.edge_count(), 0U);
    EXPECT_EQ(mesh.halfedge_count(), 0U);
    EXPECT_EQ(mesh.face_count(), 0U);

    EXPECT_EQ(mesh.vertices_size(), 3U);
    EXPECT_EQ(mesh.edges_size(), 3U);
    EXPECT_EQ(mesh.halfedges_size(), 6U);
    EXPECT_EQ(mesh.faces_size(), 1U);
    EXPECT_TRUE(mesh.vertices_size() > mesh.vertex_count());

    mesh.garbage_collection();

    EXPECT_EQ(mesh.vertices_size(), 0U);
    EXPECT_EQ(mesh.edges_size(), 0U);
    EXPECT_EQ(mesh.halfedges_size(), 0U);
    EXPECT_EQ(mesh.faces_size(), 0U);
    EXPECT_TRUE(mesh.vertices_size() == mesh.vertex_count());
}
