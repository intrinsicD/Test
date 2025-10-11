#include <gtest/gtest.h>

#include "engine/geometry/point_cloud/point_cloud.hpp"
#include "engine/geometry/octree/octree.hpp"

#include <filesystem>

namespace geo = engine::geometry;

TEST(PointCloud, StoresPointsAndProperties)
{
    geo::PointCloud cloud;

    const auto p0 = cloud.interface.add_vertex({0.0F, 0.0F, 0.0F});
    const auto p1 = cloud.interface.add_vertex({1.0F, 2.0F, 3.0F});

    EXPECT_EQ(cloud.interface.vertex_count(), 2U);
    EXPECT_TRUE(cloud.interface.is_valid(p0));
    EXPECT_FLOAT_EQ(cloud.interface.position(p1)[2], 3.0F);

    auto intensity = cloud.interface.add_vertex_property<float>("p:intensity", 0.0F);
    intensity[p1] = 5.0F;

    const auto intensity_view = cloud.interface.get_vertex_property<float>("p:intensity");
    EXPECT_FLOAT_EQ(intensity_view[p1], 5.0F);

    geo::PointCloud copy = cloud;
    EXPECT_EQ(copy.interface.vertex_count(), 2U);
    EXPECT_FLOAT_EQ(copy.interface.position(p1)[0], 1.0F);
    const auto copy_intensity = copy.interface.get_vertex_property<float>("p:intensity");
    EXPECT_FLOAT_EQ(copy_intensity[p1], 5.0F);

    cloud.interface.clear();
    EXPECT_TRUE(cloud.interface.is_empty());

    const auto p2 = cloud.interface.add_vertex({4.0F, 5.0F, 6.0F});
    EXPECT_EQ(cloud.interface.vertex_count(), 1U);
    EXPECT_FLOAT_EQ(cloud.interface.position(p2)[1], 5.0F);

    auto refreshed_intensity = cloud.interface.vertex_property<float>("p:intensity", 0.0F);
    EXPECT_FLOAT_EQ(refreshed_intensity[p2], 0.0F);
}

TEST(PointCloud, RoundTripsAsciiPLY)
{
    geo::PointCloud cloud;
    auto normals = cloud.interface.vertex_property<engine::math::vec3>("p:normal", {0.0F, 0.0F, 1.0F});
    auto colours = cloud.interface.vertex_property<engine::math::vec3>("p:color", {0.0F, 0.0F, 0.0F});
    auto alpha = cloud.interface.vertex_property<float>("p:alpha", 1.0F);
    auto intensity = cloud.interface.vertex_property<float>("p:intensity", 0.0F);

    const auto v0 = cloud.interface.add_vertex({1.0F, 2.0F, 3.0F});
    normals[v0] = engine::math::vec3{0.0F, 1.0F, 0.0F};
    colours[v0] = engine::math::vec3{0.25F, 0.5F, 0.75F};
    alpha[v0] = 0.8F;
    intensity[v0] = 2.0F;

    const auto v1 = cloud.interface.add_vertex({-4.0F, 5.0F, -6.0F});
    normals[v1] = engine::math::vec3{1.0F, 0.0F, 0.0F};
    colours[v1] = engine::math::vec3{1.0F, 0.0F, 0.0F};
    alpha[v1] = 1.0F;
    intensity[v1] = 3.0F;

    const auto file = std::filesystem::temp_directory_path() / "engine_geometry_point_cloud_roundtrip.ply";

    geo::PointCloudIOFlags flags;
    flags.format = geo::PointCloudIOFlags::Format::kPLY;
    geo::point_cloud::write(cloud.interface, file, flags);

    geo::PointCloud loaded;
    geo::point_cloud::read(loaded.interface, file);

    EXPECT_EQ(loaded.interface.vertex_count(), 2U);

    auto loaded_normals = loaded.interface.get_vertex_property<engine::math::vec3>("p:normal");
    auto loaded_colours = loaded.interface.get_vertex_property<engine::math::vec3>("p:color");
    auto loaded_alpha = loaded.interface.get_vertex_property<float>("p:alpha");
    auto loaded_intensity = loaded.interface.get_vertex_property<float>("p:intensity");

    const auto v_loaded0 = geo::VertexHandle(0U);
    const auto v_loaded1 = geo::VertexHandle(1U);

    EXPECT_FLOAT_EQ(loaded.interface.position(v_loaded0)[0], 1.0F);
    EXPECT_FLOAT_EQ(loaded.interface.position(v_loaded0)[1], 2.0F);
    EXPECT_FLOAT_EQ(loaded.interface.position(v_loaded0)[2], 3.0F);
    EXPECT_FLOAT_EQ(loaded_normals[v_loaded0][1], 1.0F);
    EXPECT_FLOAT_EQ(loaded_colours[v_loaded0][2], 0.75F);
    EXPECT_FLOAT_EQ(loaded_alpha[v_loaded0], 0.8F);
    EXPECT_FLOAT_EQ(loaded_intensity[v_loaded0], 2.0F);

    EXPECT_FLOAT_EQ(loaded.interface.position(v_loaded1)[0], -4.0F);
    EXPECT_FLOAT_EQ(loaded.interface.position(v_loaded1)[1], 5.0F);
    EXPECT_FLOAT_EQ(loaded.interface.position(v_loaded1)[2], -6.0F);
    EXPECT_FLOAT_EQ(loaded_normals[v_loaded1][0], 1.0F);
    EXPECT_FLOAT_EQ(loaded_colours[v_loaded1][0], 1.0F);
    EXPECT_FLOAT_EQ(loaded_alpha[v_loaded1], 1.0F);
    EXPECT_FLOAT_EQ(loaded_intensity[v_loaded1], 3.0F);

    std::filesystem::remove(file);
}

