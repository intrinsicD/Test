#include <gtest/gtest.h>

#include "engine/geometry/point_cloud.hpp"

namespace geo = engine::geometry;

TEST(PointCloud, StoresPointsAndProperties)
{
    geo::PointCloud cloud;

    const auto p0 = cloud.add_point({0.0F, 0.0F, 0.0F});
    const auto p1 = cloud.add_point({1.0F, 2.0F, 3.0F});

    EXPECT_EQ(cloud.point_count(), 2U);
    EXPECT_TRUE(cloud.is_valid(p0));
    EXPECT_FLOAT_EQ(cloud.position(p1)[2], 3.0F);

    auto intensity = cloud.add_point_property<float>("p:intensity", 0.0F);
    intensity[p1] = 5.0F;

    const auto intensity_view = cloud.get_point_property<float>("p:intensity");
    EXPECT_FLOAT_EQ(intensity_view[p1], 5.0F);

    geo::PointCloud copy = cloud;
    EXPECT_EQ(copy.point_count(), 2U);
    EXPECT_FLOAT_EQ(copy.position(p1)[0], 1.0F);
    const auto copy_intensity = copy.get_point_property<float>("p:intensity");
    EXPECT_FLOAT_EQ(copy_intensity[p1], 5.0F);

    cloud.clear();
    EXPECT_TRUE(cloud.is_empty());

    const auto p2 = cloud.add_point({4.0F, 5.0F, 6.0F});
    EXPECT_EQ(cloud.point_count(), 1U);
    EXPECT_FLOAT_EQ(cloud.position(p2)[1], 5.0F);

    auto refreshed_intensity = cloud.point_property<float>("p:intensity", 0.0F);
    EXPECT_FLOAT_EQ(refreshed_intensity[p2], 0.0F);
}

