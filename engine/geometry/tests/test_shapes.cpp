#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include "engine/geometry/shapes.hpp"

using engine::geometry::aabb;
using engine::geometry::bounding_aabb;
using engine::math::mat3;
using engine::math::vec3;

namespace {

void expect_vec3_eq(const vec3& actual, const vec3& expected) {
    EXPECT_FLOAT_EQ(actual[0], expected[0]);
    EXPECT_FLOAT_EQ(actual[1], expected[1]);
    EXPECT_FLOAT_EQ(actual[2], expected[2]);
}

}  // namespace

TEST(Aabb, ComputesDerivedQuantities) {
    const aabb box{vec3{-1.0f, 0.0f, 1.0f}, vec3{3.0f, 6.0f, 5.0f}};

    const vec3 expected_center{1.0f, 3.0f, 3.0f};
    expect_vec3_eq(engine::geometry::center(box), expected_center);

    const vec3 expected_size{4.0f, 6.0f, 4.0f};
    expect_vec3_eq(engine::geometry::size(box), expected_size);
    expect_vec3_eq(engine::geometry::extent(box), expected_size * 0.5f);

    EXPECT_FLOAT_EQ(engine::geometry::surface_area(box), 128.0f);
    EXPECT_FLOAT_EQ(engine::geometry::volume(box), 96.0f);

    const vec3 inside_point{0.0f, 3.0f, 3.0f};
    const vec3 outside_point{4.1f, 3.0f, 3.0f};
    EXPECT_TRUE(engine::geometry::contains(box, inside_point));
    EXPECT_TRUE(!engine::geometry::contains(box, outside_point));
}

TEST(Obb, ContainsAndBoundingBox) {
    const float angle = std::numbers::pi_v<float> * 0.25f;
    const mat3 orientation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };

    const engine::geometry::obb box{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{1.0f, 2.0f, 0.5f},
        orientation,
    };

    const vec3 local_point{0.5f, 0.5f, 0.0f};
    const vec3 inside = box.center + box.orientation * local_point;
    EXPECT_TRUE(engine::geometry::contains(box, inside));

    const vec3 outside = box.center + box.orientation * vec3{2.5f, 0.0f, 0.0f};
    EXPECT_TRUE(!engine::geometry::contains(box, outside));

    const aabb bounds = bounding_aabb(box);
    EXPECT_TRUE(std::fabs(bounds.min[0] + 2.1213205f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(bounds.min[1] + 2.1213205f) <= 1e-5f);
    EXPECT_FLOAT_EQ(bounds.min[2], -0.5f);
    EXPECT_TRUE(std::fabs(bounds.max[0] - 2.1213205f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(bounds.max[1] - 2.1213205f) <= 1e-5f);
    EXPECT_FLOAT_EQ(bounds.max[2], 0.5f);
}

TEST(Sphere, BasicMetrics) {
    const engine::geometry::sphere s{vec3{1.0f, -1.0f, 0.0f}, 2.0f};

    EXPECT_FLOAT_EQ(engine::geometry::surface_area(s), 16.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::volume(s), (32.0f / 3.0f) * std::numbers::pi_v<float>);
    const vec3 interior{1.0f, 1.0f, 0.0f};
    const vec3 exterior{1.0f, -1.0f, 3.1f};
    EXPECT_TRUE(engine::geometry::contains(s, interior));
    EXPECT_TRUE(!engine::geometry::contains(s, exterior));
}

TEST(Plane, SignedDistanceAndProjection) {
    const engine::geometry::plane p{vec3{0.0f, 1.0f, 0.0f}, -2.0f};

    EXPECT_FLOAT_EQ(engine::geometry::signed_distance(p, vec3{0.0f, 2.0f, 0.0f}), 0.0f);
    EXPECT_FLOAT_EQ(engine::geometry::signed_distance(p, vec3{0.0f, 5.0f, 0.0f}), 3.0f);

    const vec3 projected = engine::geometry::project_point(p, vec3{1.0f, 5.0f, -1.0f});
    EXPECT_FLOAT_EQ(projected[1], 2.0f);
    EXPECT_TRUE(engine::geometry::contains(p, projected));
    const vec3 offset_point{0.0f, 2.1f, 0.0f};
    EXPECT_TRUE(!engine::geometry::contains(p, offset_point, 1e-2f));
}

TEST(Ray, PointAtDistance) {
    const engine::geometry::ray r{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 2.0f, 0.0f}};
    const vec3 expected{2.0f, 4.0f, 0.0f};
    expect_vec3_eq(engine::geometry::point_at(r, 2.0f), expected);
}

TEST(Segment, LengthAndInterpolation) {
    const engine::geometry::segment s{vec3{0.0f, 0.0f, 0.0f}, vec3{3.0f, 4.0f, 0.0f}};
    EXPECT_FLOAT_EQ(engine::geometry::length(s), 5.0f);
    const vec3 midpoint{1.5f, 2.0f, 0.0f};
    expect_vec3_eq(engine::geometry::point_at(s, 0.5f), midpoint);
}

TEST(Line, Projection) {
    const engine::geometry::line l{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    const vec3 projected = engine::geometry::project_point(l, vec3{2.0f, 3.0f, -1.0f});
    expect_vec3_eq(projected, vec3{0.0f, 3.0f, 0.0f});

    const engine::geometry::line degenerate_line{vec3{1.0f, 2.0f, 3.0f}, vec3{0.0f, 0.0f, 0.0f}};
    expect_vec3_eq(engine::geometry::project_point(degenerate_line, vec3{5.0f, -1.0f, 2.0f}), degenerate_line.point);
}

TEST(Ellipsoid, ContainsAndVolume) {
    const float angle = std::numbers::pi_v<float> * 0.5f;
    const mat3 orientation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };

    const engine::geometry::ellipsoid e{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{2.0f, 1.0f, 0.5f},
        orientation,
    };

    EXPECT_FLOAT_EQ(engine::geometry::volume(e), (4.0f / 3.0f) * std::numbers::pi_v<float>);

    const vec3 inside = e.center + e.orientation * vec3{1.0f, 0.0f, 0.0f};
    const vec3 outside_point{3.0f, 0.0f, 0.0f};
    EXPECT_TRUE(engine::geometry::contains(e, inside));
    EXPECT_TRUE(!engine::geometry::contains(e, outside_point));
}

TEST(Triangle, AreaNormalAndCentroid) {
    const engine::geometry::triangle t{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 2.0f, 0.0f}};

    expect_vec3_eq(engine::geometry::normal(t), vec3{0.0f, 0.0f, 2.0f});
    expect_vec3_eq(engine::geometry::unit_normal(t), vec3{0.0f, 0.0f, 1.0f});
    EXPECT_FLOAT_EQ(engine::geometry::area(t), 1.0f);
    expect_vec3_eq(engine::geometry::centroid(t), vec3{1.0f / 3.0f, 2.0f / 3.0f, 0.0f});
}

TEST(Cylinder, AxisDerivedValuesAndContainment) {
    const engine::geometry::cylinder c{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 0.0f, 2.0f}, 1.0f, 2.0f};

    expect_vec3_eq(engine::geometry::axis_direction(c), vec3{0.0f, 0.0f, 1.0f});
    expect_vec3_eq(engine::geometry::top_center(c), vec3{0.0f, 0.0f, 2.0f});
    expect_vec3_eq(engine::geometry::bottom_center(c), vec3{0.0f, 0.0f, -2.0f});

    EXPECT_FLOAT_EQ(engine::geometry::volume(c), 4.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::lateral_surface_area(c), 8.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::surface_area(c), 10.0f * std::numbers::pi_v<float>);

    const vec3 radial_inside{0.5f, 0.0f, 1.0f};
    const vec3 radial_outside{1.1f, 0.0f, 0.0f};
    const vec3 origin{0.0f, 0.0f, 0.0f};
    EXPECT_TRUE(engine::geometry::contains(c, radial_inside));
    EXPECT_TRUE(!engine::geometry::contains(c, radial_outside));
    EXPECT_TRUE(!engine::geometry::contains(engine::geometry::cylinder{origin, vec3{0.0f}, 1.0f, 1.0f}, origin));
}

