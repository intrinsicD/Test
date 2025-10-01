#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include "engine/geometry/shapes.hpp"
#include "engine/math/matrix.hpp"

using engine::geometry::Aabb;
using engine::geometry::BoundingAabb;
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
    const Aabb box{vec3{-1.0f, 0.0f, 1.0f}, vec3{3.0f, 6.0f, 5.0f}};

    const vec3 expected_center{1.0f, 3.0f, 3.0f};
    expect_vec3_eq(engine::geometry::Center(box), expected_center);

    const vec3 expected_size{4.0f, 6.0f, 4.0f};
    expect_vec3_eq(engine::geometry::Size(box), expected_size);
    expect_vec3_eq(engine::geometry::Extent(box), expected_size * 0.5f);

    EXPECT_FLOAT_EQ(engine::geometry::SurfaceArea(box), 128.0f);
    EXPECT_FLOAT_EQ(engine::geometry::Volume(box), 96.0f);

    const vec3 inside_point{0.0f, 3.0f, 3.0f};
    const vec3 outside_point{4.1f, 3.0f, 3.0f};
    EXPECT_TRUE(engine::geometry::Contains(box, inside_point));
    EXPECT_TRUE(!engine::geometry::Contains(box, outside_point));
}

TEST(Aabb, ConversionsContainmentAndIntersections) {
    const vec3 min_corner{-1.0f, -1.0f, -1.0f};
    const vec3 max_corner{1.0f, 1.0f, 1.0f};
    const Aabb outer{min_corner, max_corner};

    const Aabb from_point_box = engine::geometry::make_aabb_from_point(vec3{0.5f, 0.5f, 0.5f});
    expect_vec3_eq(from_point_box.min, vec3{0.5f, 0.5f, 0.5f});
    expect_vec3_eq(from_point_box.max, vec3{0.5f, 0.5f, 0.5f});

    const Aabb inner = engine::geometry::make_aabb_from_center_extent(vec3{0.0f}, vec3{0.25f});
    EXPECT_TRUE(engine::geometry::Contains(outer, inner));

    const engine::geometry::Sphere inner_sphere{vec3{0.0f}, 0.5f};
    EXPECT_TRUE(engine::geometry::Contains(outer, inner_sphere));

    const engine::geometry::Obb inner_obb = engine::geometry::from_center_half_sizes(vec3{0.0f}, vec3{0.5f});
    EXPECT_TRUE(engine::geometry::Contains(outer, inner_obb));

    const engine::geometry::Sphere bounding = engine::geometry::bounding_sphere(outer);
    const Aabb inflated = engine::geometry::BoundingAabb(bounding);
    EXPECT_TRUE(engine::geometry::Contains(inflated, outer));

    EXPECT_TRUE(engine::geometry::Intersects(outer, outer));
    EXPECT_TRUE(engine::geometry::Intersects(outer, inner));
    EXPECT_TRUE(engine::geometry::Intersects(outer, bounding));
    EXPECT_TRUE(engine::geometry::Intersects(outer, inner_obb));
}

TEST(Aabb, ContainsCylinderAndEllipsoid) {
    const vec3 center{0.0f};
    const Aabb outer = engine::geometry::make_aabb_from_center_extent(center, vec3{2.0f, 2.0f, 2.0f});

    const engine::geometry::Cylinder cylinder_inside{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{0.0f, 0.0f, 1.0f},
        0.75f,
        1.0f,
    };
    EXPECT_TRUE(engine::geometry::Contains(outer, cylinder_inside));

    const engine::geometry::Cylinder cylinder_outside{
        vec3{2.5f, 0.0f, 0.0f},
        vec3{0.0f, 0.0f, 1.0f},
        0.5f,
        1.0f,
    };
    EXPECT_TRUE(!engine::geometry::Contains(outer, cylinder_outside));

    const engine::geometry::Ellipsoid ellipsoid_inside{
        vec3{0.5f, 0.25f, -0.25f},
        vec3{0.5f, 0.75f, 0.6f},
        engine::math::identity_matrix<float, 3>()
    };
    EXPECT_TRUE(engine::geometry::Contains(outer, ellipsoid_inside));

    const engine::geometry::Ellipsoid ellipsoid_outside{
        vec3{2.5f, 0.0f, 0.0f},
        vec3{0.6f, 0.6f, 0.6f},
        engine::math::identity_matrix<float, 3>()
    };
    EXPECT_TRUE(!engine::geometry::Contains(outer, ellipsoid_outside));
}

TEST(Aabb, IntersectsAdvancedShapes) {
    const Aabb box = engine::geometry::make_aabb_from_center_extent(vec3{0.0f}, vec3{1.0f});

    const engine::geometry::Cylinder cylinder{
        vec3{1.5f, 0.0f, 0.0f},
        vec3{0.0f, 0.0f, 1.0f},
        0.6f,
        0.75f,
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, cylinder));

    const engine::geometry::Cylinder far_cylinder{
        vec3{3.0f, 0.0f, 0.0f},
        vec3{0.0f, 1.0f, 0.0f},
        0.5f,
        0.5f,
    };
    EXPECT_TRUE(!engine::geometry::Intersects(box, far_cylinder));

    const float angle = std::numbers::pi_v<float> * 0.25f;
    const mat3 rotation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };

    const engine::geometry::Ellipsoid ellipsoid{
        vec3{0.9f, 0.0f, 0.0f},
        vec3{0.6f, 0.4f, 0.3f},
        rotation
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, ellipsoid));

    const engine::geometry::Ellipsoid far_ellipsoid{
        vec3{3.0f, 0.0f, 0.0f},
        vec3{0.6f, 0.4f, 0.3f},
        rotation
    };
    EXPECT_TRUE(!engine::geometry::Intersects(box, far_ellipsoid));

    const engine::geometry::Line line{
        vec3{-2.0f, -2.0f, 0.0f},
        vec3{1.0f, 1.0f, 0.0f},
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, line));

    const engine::geometry::Line distant_line{
        vec3{0.0f, 3.0f, 0.0f},
        vec3{1.0f, 0.0f, 0.0f},
    };
    EXPECT_TRUE(!engine::geometry::Intersects(box, distant_line));

    const engine::geometry::Plane plane{vec3{0.0f, 1.0f, 0.0f}, 0.0f};
    EXPECT_TRUE(engine::geometry::Intersects(box, plane));

    const engine::geometry::Plane far_plane{vec3{0.0f, 1.0f, 0.0f}, -3.0f};
    EXPECT_TRUE(!engine::geometry::Intersects(box, far_plane));

    const engine::geometry::Ray ray{
        vec3{-3.0f, 0.2f, 0.0f},
        vec3{1.0f, 0.0f, 0.0f},
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, ray));

    const engine::geometry::Ray miss_ray{
        vec3{-3.0f, 3.0f, 0.0f},
        vec3{1.0f, 0.0f, 0.0f},
    };
    EXPECT_TRUE(!engine::geometry::Intersects(box, miss_ray));

    const engine::geometry::Segment segment{
        vec3{-3.0f, 0.0f, 0.0f},
        vec3{0.5f, 0.0f, 0.0f},
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, segment));

    const engine::geometry::Segment miss_segment{
        vec3{-3.0f, 2.0f, 0.0f},
        vec3{-1.5f, 2.0f, 0.0f},
    };
    EXPECT_TRUE(!engine::geometry::Intersects(box, miss_segment));

    const engine::geometry::Triangle triangle{
        vec3{0.0f, 2.0f, 0.0f},
        vec3{0.0f, -2.0f, 0.0f},
        vec3{0.0f, 0.0f, 2.0f},
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, triangle));

    const engine::geometry::Triangle far_triangle{
        vec3{3.0f, 3.0f, 3.0f},
        vec3{4.0f, 3.0f, 3.0f},
        vec3{3.5f, 4.0f, 3.0f},
    };
    EXPECT_TRUE(!engine::geometry::Intersects(box, far_triangle));
}

TEST(Aabb, BoundingVolumesForCompositeShapes) {
    const engine::geometry::Cylinder cylinder{
        vec3{0.0f, 1.0f, -1.0f},
        vec3{1.0f, 1.0f, 0.0f},
        1.0f,
        2.0f,
    };
    const Aabb cylinder_bounds = engine::geometry::BoundingAabb(cylinder);
    const vec3 cylinder_extent = engine::geometry::Extent(cylinder_bounds);
    EXPECT_TRUE(std::fabs(cylinder_bounds.min[2] + 2.0f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(cylinder_bounds.max[2] - 0.0f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(cylinder_extent[0] - 2.1213203f) <= 1e-4f);
    EXPECT_TRUE(std::fabs(cylinder_extent[1] - 2.1213203f) <= 1e-4f);

    const float angle = std::numbers::pi_v<float> * 0.25f;
    const mat3 rotation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    const engine::geometry::Ellipsoid ellipsoid{
        vec3{2.0f, 0.0f, 1.0f},
        vec3{1.0f, 2.0f, 0.5f},
        rotation,
    };
    const Aabb ellipsoid_bounds = engine::geometry::BoundingAabb(ellipsoid);
    const vec3 ellipsoid_extent = engine::geometry::Extent(ellipsoid_bounds);
    EXPECT_TRUE(std::fabs(ellipsoid_extent[0] - 2.1213203f) <= 1e-4f);
    EXPECT_TRUE(std::fabs(ellipsoid_extent[1] - 2.1213203f) <= 1e-4f);
    EXPECT_FLOAT_EQ(ellipsoid_extent[2], 0.5f);

    const engine::geometry::Segment segment{vec3{-1.0f, 2.0f, 3.0f}, vec3{4.0f, -1.0f, 5.0f}};
    const Aabb segment_bounds = engine::geometry::BoundingAabb(segment);
    expect_vec3_eq(segment_bounds.min, vec3{-1.0f, -1.0f, 3.0f});
    expect_vec3_eq(segment_bounds.max, vec3{4.0f, 2.0f, 5.0f});

    const engine::geometry::Triangle triangle{
        vec3{1.0f, -2.0f, 0.0f},
        vec3{-3.0f, 4.0f, 2.0f},
        vec3{0.5f, 1.5f, -1.0f},
    };
    const Aabb triangle_bounds = engine::geometry::BoundingAabb(triangle);
    expect_vec3_eq(triangle_bounds.min, vec3{-3.0f, -2.0f, -1.0f});
    expect_vec3_eq(triangle_bounds.max, vec3{1.0f, 4.0f, 2.0f});
}

TEST(Obb, ContainsAndBoundingBox) {
    const float angle = std::numbers::pi_v<float> * 0.25f;
    const mat3 orientation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };

    const engine::geometry::Obb box{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{1.0f, 2.0f, 0.5f},
        orientation,
    };

    const vec3 local_point{0.5f, 0.5f, 0.0f};
    const vec3 inside = box.center + box.orientation * local_point;
    EXPECT_TRUE(engine::geometry::Contains(box, inside));

    const vec3 outside = box.center + box.orientation * vec3{2.5f, 0.0f, 0.0f};
    EXPECT_TRUE(!engine::geometry::Contains(box, outside));

    const Aabb bounds = BoundingAabb(box);
    EXPECT_TRUE(std::fabs(bounds.min[0] + 2.1213205f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(bounds.min[1] + 2.1213205f) <= 1e-5f);
    EXPECT_FLOAT_EQ(bounds.min[2], -0.5f);
    EXPECT_TRUE(std::fabs(bounds.max[0] - 2.1213205f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(bounds.max[1] - 2.1213205f) <= 1e-5f);
    EXPECT_FLOAT_EQ(bounds.max[2], 0.5f);
}

TEST(Obb, IntersectionsAndConversions) {
    const engine::geometry::Obb base = engine::geometry::from_center_half_sizes(vec3{0.0f}, vec3{1.0f, 2.0f, 0.5f});
    const engine::geometry::Obb same_space = engine::geometry::from_center_half_sizes(vec3{0.5f, 0.0f, 0.0f}, vec3{0.25f, 0.25f, 0.25f});
    EXPECT_TRUE(engine::geometry::Contains(base, engine::geometry::from_aabb(engine::geometry::BoundingAabb(same_space))));
    EXPECT_TRUE(engine::geometry::Contains(base, same_space));

    const engine::geometry::Sphere s = engine::geometry::bounding_sphere(base);
    EXPECT_TRUE(engine::geometry::Intersects(base, s));

    const vec3 offset{3.0f, 0.0f, 0.0f};
    const engine::geometry::Obb far_box{offset, vec3{0.5f}, engine::math::identity_matrix<float, 3>()};
    EXPECT_TRUE(engine::geometry::Intersects(base, base));
    EXPECT_TRUE(!engine::geometry::Intersects(base, far_box));
}

TEST(Sphere, BasicMetrics) {
    const engine::geometry::Sphere s{vec3{1.0f, -1.0f, 0.0f}, 2.0f};

    EXPECT_FLOAT_EQ(engine::geometry::surface_area(s), 16.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::volume(s), (32.0f / 3.0f) * std::numbers::pi_v<float>);
    const vec3 interior{1.0f, 1.0f, 0.0f};
    const vec3 exterior{1.0f, -1.0f, 3.1f};
    EXPECT_TRUE(engine::geometry::Contains(s, interior));
    EXPECT_TRUE(!engine::geometry::Contains(s, exterior));
}

TEST(Sphere, ContainmentAndConversions) {
    const engine::geometry::Sphere s = engine::geometry::make_sphere_from_point(vec3{1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(s.radius, 0.0f);
    expect_vec3_eq(s.center, vec3{1.0f, 2.0f, 3.0f});

    const Aabb box = engine::geometry::make_aabb_from_center_extent(vec3{1.0f, 2.0f, 3.0f}, vec3{1.0f});
    const engine::geometry::Sphere enclosing = engine::geometry::bounding_sphere(box);
    EXPECT_TRUE(engine::geometry::Contains(enclosing, box));

    const engine::geometry::Obb o = engine::geometry::from_center_half_sizes(vec3{1.0f, 2.0f, 3.0f}, vec3{0.5f, 0.75f, 1.0f});
    EXPECT_TRUE(engine::geometry::Contains(enclosing, o));

    const engine::geometry::Sphere another{vec3{3.0f, 2.0f, 3.0f}, 1.0f};
    EXPECT_TRUE(engine::geometry::Intersects(enclosing, another));
    EXPECT_TRUE(engine::geometry::Contains(enclosing, s));
}

TEST(Plane, SignedDistanceAndProjection) {
    const engine::geometry::Plane p{vec3{0.0f, 1.0f, 0.0f}, -2.0f};

    EXPECT_FLOAT_EQ(engine::geometry::signed_distance(p, vec3{0.0f, 2.0f, 0.0f}), 0.0f);
    EXPECT_FLOAT_EQ(engine::geometry::signed_distance(p, vec3{0.0f, 5.0f, 0.0f}), 3.0f);

    const vec3 projected = engine::geometry::project_point(p, vec3{1.0f, 5.0f, -1.0f});
    EXPECT_FLOAT_EQ(projected[1], 2.0f);
    EXPECT_TRUE(engine::geometry::Contains(p, projected));
    const vec3 offset_point{0.0f, 2.1f, 0.0f};
    EXPECT_TRUE(!engine::geometry::Contains(p, offset_point, 1e-2f));
}

TEST(Plane, Intersections) {
    const engine::geometry::Plane p{vec3{0.0f, 1.0f, 0.0f}, -1.0f};
    const engine::geometry::Ray r{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    float t = 0.0f;
    EXPECT_TRUE(engine::geometry::Intersects(p, r, t));
    EXPECT_FLOAT_EQ(t, 3.0f);

    const engine::geometry::Segment s{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 2.0f, 0.0f}};
    EXPECT_TRUE(engine::geometry::Intersects(p, s, t));
    EXPECT_FLOAT_EQ(t, 0.75f);

    const engine::geometry::Line l{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    EXPECT_TRUE(engine::geometry::Intersects(p, l, t));
    EXPECT_FLOAT_EQ(t, 3.0f);
}

TEST(Ray, PointAtDistance) {
    const engine::geometry::Ray r{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 2.0f, 0.0f}};
    const vec3 expected{2.0f, 4.0f, 0.0f};
    expect_vec3_eq(engine::geometry::point_at(r, 2.0f), expected);
}

TEST(Ray, Intersections) {
    const engine::geometry::Ray r{vec3{-2.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}};
    const Aabb box = engine::geometry::make_aabb_from_center_extent(vec3{0.0f}, vec3{1.0f});
    float t_min = 0.0f;
    float t_max = 0.0f;
    EXPECT_TRUE(engine::geometry::Intersects(r, box, t_min, t_max));
    EXPECT_FLOAT_EQ(t_min, 1.0f);
    EXPECT_FLOAT_EQ(t_max, 3.0f);

    const engine::geometry::Sphere s{vec3{0.0f}, 1.0f};
    float t = 0.0f;
    EXPECT_TRUE(engine::geometry::Intersects(r, s, t));
    EXPECT_FLOAT_EQ(t, 1.0f);
}

TEST(Segment, LengthAndInterpolation) {
    const engine::geometry::Segment s{vec3{0.0f, 0.0f, 0.0f}, vec3{3.0f, 4.0f, 0.0f}};
    EXPECT_FLOAT_EQ(engine::geometry::length(s), 5.0f);
    const vec3 midpoint{1.5f, 2.0f, 0.0f};
    expect_vec3_eq(engine::geometry::point_at(s, 0.5f), midpoint);
}

TEST(Line, Projection) {
    const engine::geometry::Line l{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    const vec3 projected = engine::geometry::project_point(l, vec3{2.0f, 3.0f, -1.0f});
    expect_vec3_eq(projected, vec3{0.0f, 3.0f, 0.0f});

    const engine::geometry::Line degenerate_line{vec3{1.0f, 2.0f, 3.0f}, vec3{0.0f, 0.0f, 0.0f}};
    expect_vec3_eq(engine::geometry::project_point(degenerate_line, vec3{5.0f, -1.0f, 2.0f}), degenerate_line.point);
}

TEST(Ellipsoid, ContainsAndVolume) {
    const float angle = std::numbers::pi_v<float> * 0.5f;
    const mat3 orientation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };

    const engine::geometry::Ellipsoid e{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{2.0f, 1.0f, 0.5f},
        orientation,
    };

    EXPECT_FLOAT_EQ(engine::geometry::volume(e), (4.0f / 3.0f) * std::numbers::pi_v<float>);

    const vec3 inside = e.center + e.orientation * vec3{1.0f, 0.0f, 0.0f};
    const vec3 outside_point{3.0f, 0.0f, 0.0f};
    EXPECT_TRUE(engine::geometry::Contains(e, inside));
    EXPECT_TRUE(!engine::geometry::Contains(e, outside_point));
}

TEST(Triangle, AreaNormalAndCentroid) {
    const engine::geometry::Triangle t{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 2.0f, 0.0f}};

    expect_vec3_eq(engine::geometry::normal(t), vec3{0.0f, 0.0f, 2.0f});
    expect_vec3_eq(engine::geometry::unit_normal(t), vec3{0.0f, 0.0f, 1.0f});
    EXPECT_FLOAT_EQ(engine::geometry::area(t), 1.0f);
    expect_vec3_eq(engine::geometry::centroid(t), vec3{1.0f / 3.0f, 2.0f / 3.0f, 0.0f});
}

TEST(Cylinder, AxisDerivedValuesAndContainment) {
    const engine::geometry::Cylinder c{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 0.0f, 2.0f}, 1.0f, 2.0f};

    expect_vec3_eq(engine::geometry::axis_direction(c), vec3{0.0f, 0.0f, 1.0f});
    expect_vec3_eq(engine::geometry::top_center(c), vec3{0.0f, 0.0f, 2.0f});
    expect_vec3_eq(engine::geometry::bottom_center(c), vec3{0.0f, 0.0f, -2.0f});

    EXPECT_FLOAT_EQ(engine::geometry::volume(c), 4.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::lateral_surface_area(c), 8.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::surface_area(c), 10.0f * std::numbers::pi_v<float>);

    const vec3 radial_inside{0.5f, 0.0f, 1.0f};
    const vec3 radial_outside{1.1f, 0.0f, 0.0f};
    const vec3 origin{0.0f, 0.0f, 0.0f};
    EXPECT_TRUE(engine::geometry::Contains(c, radial_inside));
    EXPECT_TRUE(!engine::geometry::Contains(c, radial_outside));
    EXPECT_TRUE(!engine::geometry::Contains(engine::geometry::Cylinder{origin, vec3{0.0f}, 1.0f, 1.0f}, origin));
}

TEST(Cylinder, IntersectsSphere) {
    const engine::geometry::Cylinder c{vec3{0.0f}, vec3{0.0f, 0.0f, 4.0f}, 1.0f, 2.0f};
    const engine::geometry::Sphere touching{vec3{0.0f, 0.0f, 3.0f}, 1.0f};
    const engine::geometry::Sphere separate{vec3{5.0f, 0.0f, 0.0f}, 1.0f};
    EXPECT_TRUE(engine::geometry::Intersects(c, touching));
    EXPECT_TRUE(!engine::geometry::Intersects(c, separate));
}

