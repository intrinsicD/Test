#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include "engine/geometry/shapes.hpp"
#include "engine/math/matrix.hpp"

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

TEST(Aabb, ConversionsContainmentAndIntersections) {
    const vec3 min_corner{-1.0f, -1.0f, -1.0f};
    const vec3 max_corner{1.0f, 1.0f, 1.0f};
    const aabb outer{min_corner, max_corner};

    const aabb from_point_box = engine::geometry::make_aabb_from_point(vec3{0.5f, 0.5f, 0.5f});
    expect_vec3_eq(from_point_box.min, vec3{0.5f, 0.5f, 0.5f});
    expect_vec3_eq(from_point_box.max, vec3{0.5f, 0.5f, 0.5f});

    const aabb inner = engine::geometry::make_aabb_from_center_extent(vec3{0.0f}, vec3{0.25f});
    EXPECT_TRUE(engine::geometry::contains(outer, inner));

    const engine::geometry::sphere inner_sphere{vec3{0.0f}, 0.5f};
    EXPECT_TRUE(engine::geometry::contains(outer, inner_sphere));

    const engine::geometry::obb inner_obb = engine::geometry::from_center_half_sizes(vec3{0.0f}, vec3{0.5f});
    EXPECT_TRUE(engine::geometry::contains(outer, inner_obb));

    const engine::geometry::sphere bounding = engine::geometry::bounding_sphere(outer);
    const aabb inflated = engine::geometry::bounding_aabb(bounding);
    EXPECT_TRUE(engine::geometry::contains(inflated, outer));

    EXPECT_TRUE(engine::geometry::intersects(outer, outer));
    EXPECT_TRUE(engine::geometry::intersects(outer, inner));
    EXPECT_TRUE(engine::geometry::intersects(outer, bounding));
    EXPECT_TRUE(engine::geometry::intersects(outer, inner_obb));
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

TEST(Obb, IntersectionsAndConversions) {
    const engine::geometry::obb base = engine::geometry::from_center_half_sizes(vec3{0.0f}, vec3{1.0f, 2.0f, 0.5f});
    const engine::geometry::obb same_space = engine::geometry::from_center_half_sizes(vec3{0.5f, 0.0f, 0.0f}, vec3{0.25f, 0.25f, 0.25f});
    EXPECT_TRUE(engine::geometry::contains(base, engine::geometry::from_aabb(engine::geometry::bounding_aabb(same_space))));
    EXPECT_TRUE(engine::geometry::contains(base, same_space));

    const engine::geometry::sphere s = engine::geometry::bounding_sphere(base);
    EXPECT_TRUE(engine::geometry::intersects(base, s));

    const vec3 offset{3.0f, 0.0f, 0.0f};
    const engine::geometry::obb far_box{offset, vec3{0.5f}, engine::math::identity_matrix<float, 3>()};
    EXPECT_TRUE(engine::geometry::intersects(base, base));
    EXPECT_TRUE(!engine::geometry::intersects(base, far_box));
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

TEST(Sphere, ContainmentAndConversions) {
    const engine::geometry::sphere s = engine::geometry::make_sphere_from_point(vec3{1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(s.radius, 0.0f);
    expect_vec3_eq(s.center, vec3{1.0f, 2.0f, 3.0f});

    const aabb box = engine::geometry::make_aabb_from_center_extent(vec3{1.0f, 2.0f, 3.0f}, vec3{1.0f});
    const engine::geometry::sphere enclosing = engine::geometry::bounding_sphere(box);
    EXPECT_TRUE(engine::geometry::contains(enclosing, box));

    const engine::geometry::obb o = engine::geometry::from_center_half_sizes(vec3{1.0f, 2.0f, 3.0f}, vec3{0.5f, 0.75f, 1.0f});
    EXPECT_TRUE(engine::geometry::contains(enclosing, o));

    const engine::geometry::sphere another{vec3{3.0f, 2.0f, 3.0f}, 1.0f};
    EXPECT_TRUE(engine::geometry::intersects(enclosing, another));
    EXPECT_TRUE(engine::geometry::contains(enclosing, s));
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

TEST(Plane, Intersections) {
    const engine::geometry::plane p{vec3{0.0f, 1.0f, 0.0f}, -1.0f};
    const engine::geometry::ray r{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    float t = 0.0f;
    EXPECT_TRUE(engine::geometry::intersects(p, r, t));
    EXPECT_FLOAT_EQ(t, 3.0f);

    const engine::geometry::segment s{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 2.0f, 0.0f}};
    EXPECT_TRUE(engine::geometry::intersects(p, s, t));
    EXPECT_FLOAT_EQ(t, 0.75f);

    const engine::geometry::line l{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    EXPECT_TRUE(engine::geometry::intersects(p, l, t));
    EXPECT_FLOAT_EQ(t, 3.0f);
}

TEST(Ray, PointAtDistance) {
    const engine::geometry::ray r{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 2.0f, 0.0f}};
    const vec3 expected{2.0f, 4.0f, 0.0f};
    expect_vec3_eq(engine::geometry::point_at(r, 2.0f), expected);
}

TEST(Ray, Intersections) {
    const engine::geometry::ray r{vec3{-2.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}};
    const aabb box = engine::geometry::make_aabb_from_center_extent(vec3{0.0f}, vec3{1.0f});
    float t_min = 0.0f;
    float t_max = 0.0f;
    EXPECT_TRUE(engine::geometry::intersects(r, box, t_min, t_max));
    EXPECT_FLOAT_EQ(t_min, 1.0f);
    EXPECT_FLOAT_EQ(t_max, 3.0f);

    const engine::geometry::sphere s{vec3{0.0f}, 1.0f};
    float t = 0.0f;
    EXPECT_TRUE(engine::geometry::intersects(r, s, t));
    EXPECT_FLOAT_EQ(t, 1.0f);
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

TEST(Cylinder, IntersectsSphere) {
    const engine::geometry::cylinder c{vec3{0.0f}, vec3{0.0f, 0.0f, 4.0f}, 1.0f, 2.0f};
    const engine::geometry::sphere touching{vec3{0.0f, 0.0f, 3.0f}, 1.0f};
    const engine::geometry::sphere separate{vec3{5.0f, 0.0f, 0.0f}, 1.0f};
    EXPECT_TRUE(engine::geometry::intersects(c, touching));
    EXPECT_TRUE(!engine::geometry::intersects(c, separate));
}

