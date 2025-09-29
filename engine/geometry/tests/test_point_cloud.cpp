#include <gtest/gtest.h>

#include "engine/geometry/point_cloud/point_cloud.hpp"

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

