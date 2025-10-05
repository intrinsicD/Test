#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <numbers>
#include <engine/math/utils_rotation.hpp>

#include "engine/geometry/shapes.hpp"
#include "engine/math/quaternion.hpp"
#include "engine/math/utils.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"

using engine::geometry::Aabb;
using engine::geometry::BoundingAabb;
using engine::geometry::Triangle;
using engine::math::mat3;
using engine::math::mat4;
using engine::math::vec3;
using engine::math::vec4;
using engine::math::quat;

namespace
{
    void expect_vec3_eq(const vec3& actual, const vec3& expected)
    {
        EXPECT_FLOAT_EQ(actual[0], expected[0]);
        EXPECT_FLOAT_EQ(actual[1], expected[1]);
        EXPECT_FLOAT_EQ(actual[2], expected[2]);
    }
} // namespace

TEST(Aabb, ComputesDerivedQuantities)
{
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

TEST(Aabb, ConversionsContainmentAndIntersections)
{
    const vec3 min_corner{-1.0f, -1.0f, -1.0f};
    const vec3 max_corner{1.0f, 1.0f, 1.0f};
    const Aabb outer{min_corner, max_corner};

    const Aabb from_point_box = engine::geometry::BoundingAabb(vec3{0.5f, 0.5f, 0.5f});
    expect_vec3_eq(from_point_box.min, vec3{0.5f, 0.5f, 0.5f});
    expect_vec3_eq(from_point_box.max, vec3{0.5f, 0.5f, 0.5f});

    const Aabb inner = engine::geometry::MakeAabbFromCenterExtent(vec3{0.0f}, vec3{0.25f});
    EXPECT_TRUE(engine::geometry::Contains(outer, inner));

    const engine::geometry::Sphere inner_sphere{vec3{0.0f}, 0.5f};
    EXPECT_TRUE(engine::geometry::Contains(outer, inner_sphere));

    const engine::geometry::Obb inner_obb = engine::geometry::MakeObbFromCenterHalfSizes(vec3{0.0f}, vec3{0.5f}, {});
    EXPECT_TRUE(engine::geometry::Contains(outer, inner_obb));

    const engine::geometry::Sphere bounding = engine::geometry::BoundingSphere(outer);
    const Aabb inflated = engine::geometry::BoundingAabb(bounding);
    EXPECT_TRUE(engine::geometry::Contains(inflated, outer));

    EXPECT_TRUE(engine::geometry::Intersects(outer, outer));
    EXPECT_TRUE(engine::geometry::Intersects(outer, inner));
    EXPECT_TRUE(engine::geometry::Intersects(outer, bounding));
    EXPECT_TRUE(engine::geometry::Intersects(outer, inner_obb));
}

TEST(Aabb, ContainsCylinderAndEllipsoid)
{
    const vec3 center{0.0f};
    const Aabb outer = engine::geometry::MakeAabbFromCenterExtent(center, vec3{2.0f, 2.0f, 2.0f});

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
        quat::Identity()
    };
    EXPECT_TRUE(engine::geometry::Contains(outer, ellipsoid_inside));

    const engine::geometry::Ellipsoid ellipsoid_outside{
        vec3{2.5f, 0.0f, 0.0f},
        vec3{0.6f, 0.6f, 0.6f},
        quat::Identity()
    };
    EXPECT_TRUE(!engine::geometry::Contains(outer, ellipsoid_outside));
}


TEST(Aabb, IntersectsAdvancedShapes)
{
    const Aabb box = engine::geometry::MakeAabbFromCenterExtent(vec3{0.0f}, vec3{1.0f});

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
    const quat rotation_quat = engine::math::utils::to_quaternion(rotation);

    const engine::geometry::Ellipsoid ellipsoid{
        vec3{0.9f, 0.0f, 0.0f},
        vec3{0.6f, 0.4f, 0.3f},
        rotation_quat
    };
    EXPECT_TRUE(engine::geometry::Intersects(box, ellipsoid));

    const engine::geometry::Ellipsoid far_ellipsoid{
        vec3{3.0f, 0.0f, 0.0f},
        vec3{0.6f, 0.4f, 0.3f},
        rotation_quat
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

TEST(Aabb, BoundingVolumesForCompositeShapes)
{
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
    const quat rotation_quat = engine::math::from_rotation_matrix(rotation);
    const engine::geometry::Ellipsoid ellipsoid{
        vec3{2.0f, 0.0f, 1.0f},
        vec3{1.0f, 2.0f, 0.5f},
        rotation_quat,
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

TEST(Obb, ContainsAndBoundingBox)
{
    const float angle = std::numbers::pi_v<float> * 0.25f;

    const mat3 orientation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    const quat orientation_quat = engine::math::from_rotation_matrix(orientation);

    const engine::geometry::Obb box{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{1.0f, 2.0f, 0.5f},
        orientation_quat,
    };

    const vec3 local_point{0.5f, 0.5f, 0.0f};
    const vec3 inside = box.center + vec3(engine::math::utils::to_rotation_matrix(box.orientation) * local_point);
    EXPECT_TRUE(engine::geometry::Contains(box, inside));

    const vec3 outside = box.center + vec3(
        engine::math::utils::to_rotation_matrix(box.orientation) * vec3{2.5f, 0.0f, 0.0f});
    EXPECT_TRUE(!engine::geometry::Contains(box, outside));

    const Aabb bounds = BoundingAabb(box);
    EXPECT_TRUE(std::fabs(bounds.min[0] + 2.1213205f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(bounds.min[1] + 2.1213205f) <= 1e-5f);
    EXPECT_FLOAT_EQ(bounds.min[2], -0.5f);
    EXPECT_TRUE(std::fabs(bounds.max[0] - 2.1213205f) <= 1e-5f);
    EXPECT_TRUE(std::fabs(bounds.max[1] - 2.1213205f) <= 1e-5f);
    EXPECT_FLOAT_EQ(bounds.max[2], 0.5f);
}

TEST(Obb, IntersectionsAndConversions)
{
    const engine::geometry::Obb base = engine::geometry::MakeObbFromCenterHalfSizes(
        vec3{0.0f}, vec3{1.0f, 2.0f, 0.5f}, quat::Identity());
    const engine::geometry::Obb same_space = engine::geometry::MakeObbFromCenterHalfSizes(
        vec3{0.5f, 0.0f, 0.0f}, vec3{0.25f, 0.25f, 0.25f}, quat::Identity());
    EXPECT_TRUE(
        engine::geometry::Contains(base, engine::geometry::BoundingObb(engine::geometry::BoundingAabb(same_space))));
    EXPECT_TRUE(engine::geometry::Contains(base, same_space));

    const engine::geometry::Sphere s = engine::geometry::BoundingSphere(base);
    EXPECT_TRUE(engine::geometry::Intersects(base, s));

    const vec3 offset{3.0f, 0.0f, 0.0f};
    const engine::geometry::Obb far_box{offset, vec3{0.5f}, quat::Identity()};
    EXPECT_TRUE(engine::geometry::Intersects(base, base));
    EXPECT_TRUE(!engine::geometry::Intersects(base, far_box));
}

TEST(Obb, ClosestPointAndDistance)
{
    const quat orientation = engine::math::from_angle_axis(std::numbers::pi_v<float> / 4.0f, vec3{0.0f, 0.0f, 1.0f});
    const engine::geometry::Obb box{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 2.0f, 0.5f}, orientation};
    const mat3 rotation = engine::math::utils::to_rotation_matrix(orientation);

    const vec3 point{3.0f, 0.0f, 0.25f};
    const vec3 local = engine::math::transpose(rotation) * (point - box.center);
    const vec3 clamped{
        engine::math::utils::clamp(local[0], -box.half_sizes[0], box.half_sizes[0]),
        engine::math::utils::clamp(local[1], -box.half_sizes[1], box.half_sizes[1]),
        engine::math::utils::clamp(local[2], -box.half_sizes[2], box.half_sizes[2])
    };
    const vec3 expected = box.center + rotation * clamped;

    expect_vec3_eq(engine::geometry::ClosestPoint(box, point), expected);
    EXPECT_TRUE(engine::math::utils::nearly_equal(engine::geometry::SquaredDistance(box, point),
        static_cast<double>(engine::math::length_squared(point - expected)), 1e-5));
}

TEST(Obb, BoundingObbWithTransform)
{
    const engine::geometry::Obb base{vec3{0.5f, -0.5f, 0.0f}, vec3{1.0f, 0.5f, 0.25f}, {}};
    const quat rotation_quat = engine::math::from_angle_axis(std::numbers::pi_v<float> / 2.0f, vec3{0.0f, 0.0f, 1.0f});
    const mat3 rotation = engine::math::utils::to_rotation_matrix(rotation_quat);

    mat4 transform = engine::math::identity_matrix<float, 4>();
    for (std::size_t r = 0; r < 3; ++r)
    {
        for (std::size_t c = 0; c < 3; ++c)
        {
            transform[r][c] = rotation[r][c];
        }
    }
    transform[0][3] = 1.0f;
    transform[1][3] = 2.0f;
    transform[2][3] = -1.0f;

    const engine::geometry::Obb transformed = engine::geometry::BoundingObb(base, transform);
    const vec3 expected_center = vec3{
        rotation[0][0] * base.center[0] + rotation[0][1] * base.center[1] + rotation[0][2] * base.center[2] + 1.0f,
        rotation[1][0] * base.center[0] + rotation[1][1] * base.center[1] + rotation[1][2] * base.center[2] + 2.0f,
        rotation[2][0] * base.center[0] + rotation[2][1] * base.center[1] + rotation[2][2] * base.center[2] - 1.0f
    };
    expect_vec3_eq(transformed.center, expected_center);

    const mat3 transformed_rotation = engine::math::utils::to_rotation_matrix(transformed.orientation);
    for (std::size_t r = 0; r < 3; ++r)
    {
        for (std::size_t c = 0; c < 3; ++c)
        {
            EXPECT_TRUE(engine::math::utils::nearly_equal(transformed_rotation[r][c], rotation[r][c], 1e-5f));
        }
    }

    expect_vec3_eq(transformed.half_sizes, base.half_sizes);
}

TEST(Obb, BoundingObbFromPoints)
{
    std::array<vec3, 4> points{
        vec3{-1.0f, -1.0f, 0.0f},
        vec3{1.0f, -1.0f, 0.0f},
        vec3{-1.0f, 1.0f, 0.0f},
        vec3{1.0f, 1.0f, 0.0f}
    };
    const engine::geometry::Obb bounds = engine::geometry::BoundingObb(points);
    expect_vec3_eq(bounds.center, vec3{0.0f, 0.0f, 0.0f});
    expect_vec3_eq(bounds.half_sizes, vec3{1.0f, 1.0f, 0.0f});
}

TEST(Sphere, BasicMetrics)
{
    const engine::geometry::Sphere s{vec3{1.0f, -1.0f, 0.0f}, 2.0f};

    EXPECT_FLOAT_EQ(engine::geometry::SurfaceArea(s), 16.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::Volume(s), (32.0f / 3.0f) * std::numbers::pi_v<float>);
    const vec3 interior{1.0f, 1.0f, 0.0f};
    const vec3 exterior{1.0f, -1.0f, 3.1f};
    EXPECT_TRUE(engine::geometry::Contains(s, interior));
    EXPECT_TRUE(!engine::geometry::Contains(s, exterior));
}

TEST(Sphere, ContainmentAndConversions)
{
    const engine::geometry::Sphere s = engine::geometry::BoundingSphere(vec3{1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(s.radius, 0.0f);
    expect_vec3_eq(s.center, vec3{1.0f, 2.0f, 3.0f});

    const Aabb box = engine::geometry::MakeAabbFromCenterExtent(vec3{1.0f, 2.0f, 3.0f}, vec3{1.0f});
    const engine::geometry::Sphere enclosing = engine::geometry::BoundingSphere(box);
    EXPECT_TRUE(engine::geometry::Contains(enclosing, box));

    const engine::geometry::Obb o = engine::geometry::MakeObbFromCenterHalfSizes(
        vec3{1.0f, 2.0f, 3.0f}, vec3{0.5f, 0.75f, 1.0f}, quat::Identity());
    EXPECT_TRUE(engine::geometry::Contains(enclosing, o));

    const engine::geometry::Sphere another{vec3{3.0f, 2.0f, 3.0f}, 1.0f};
    EXPECT_TRUE(engine::geometry::Intersects(enclosing, another));
    EXPECT_TRUE(engine::geometry::Contains(enclosing, s));
}

TEST(Ellipsoid, ClosestPointAndDistance)
{
    const engine::geometry::Ellipsoid ellipsoid{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{2.0f, 1.0f, 1.5f},
        quat::Identity()
    };

    const vec3 outside{4.0f, 0.0f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(ellipsoid, outside), vec3{2.0f, 0.0f, 0.0f});
    EXPECT_TRUE(engine::math::utils::nearly_equal(engine::geometry::SquaredDistance(ellipsoid, outside), 4.0, 1e-5));

    const vec3 inside{1.0f, 0.0f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(ellipsoid, inside), inside);

    const quat rot = engine::math::from_angle_axis(std::numbers::pi_v<float> / 2.0f, vec3{0.0f, 0.0f, 1.0f});
    const engine::geometry::Ellipsoid rotated{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{2.0f, 1.0f, 1.0f},
        rot
    };
    const vec3 axis_point{0.0f, 3.0f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(rotated, axis_point), vec3{0.0f, 2.0f, 0.0f});
}

TEST(Plane, SignedDistanceAndProjection)
{
    const engine::geometry::Plane p{vec3{0.0f, 1.0f, 0.0f}, -2.0f};

    EXPECT_FLOAT_EQ(engine::geometry::SignedDistance(p, vec3{0.0f, 2.0f, 0.0f}), 0.0f);
    EXPECT_FLOAT_EQ(engine::geometry::SignedDistance(p, vec3{0.0f, 5.0f, 0.0f}), 3.0f);

    const vec3 projected = engine::geometry::ClosestPoint(p, vec3{1.0f, 5.0f, -1.0f});
    EXPECT_FLOAT_EQ(projected[1], 2.0f);
    EXPECT_TRUE(engine::geometry::Contains(p, projected));
    const vec3 offset_point{0.0f, 2.1f, 0.0f};
    EXPECT_TRUE(!engine::geometry::Contains(p, offset_point, 1e-2f));
}

TEST(Plane, Intersections)
{
    const engine::geometry::Plane p{vec3{0.0f, 1.0f, 0.0f}, -1.0f};
    const engine::geometry::Ray r{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    {
        engine::geometry::Result result{};
        EXPECT_TRUE(engine::geometry::Intersects(p, r, &result));
        EXPECT_FLOAT_EQ(result.t, 3.0f);
    }

    const engine::geometry::Segment s{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 2.0f, 0.0f}};
    {
        engine::geometry::Result result{};
        EXPECT_TRUE(engine::geometry::Intersects(p, s, &result));
        EXPECT_FLOAT_EQ(result.t, 0.75f);
    }
    const engine::geometry::Line l{vec3{0.0f, -2.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    {
        engine::geometry::Result result{};
        EXPECT_TRUE(engine::geometry::Intersects(p, l, &result));
        EXPECT_FLOAT_EQ(result.t, 3.0f);
    }
}

TEST(Ray, PointAtDistance)
{
    const engine::geometry::Ray r{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 2.0f, 0.0f}};
    const vec3 expected{2.0f, 4.0f, 0.0f};
    expect_vec3_eq(engine::geometry::PointAt(r, 2.0f), expected);
}

TEST(Ray, ClosestPointAndDistance)
{
    const engine::geometry::Ray r{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}};
    const vec3 point{2.0f, 3.0f, 0.0f};
    const vec3 expected{2.0f, 0.0f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(r, point), expected);
    EXPECT_EQ(engine::geometry::SquaredDistance(r, point), 9.0);

    const vec3 behind{-1.0f, 0.5f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(r, behind), r.origin);
}

TEST(Ray, Intersections)
{
    const engine::geometry::Ray r{vec3{-2.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}};
    const Aabb box = engine::geometry::MakeAabbFromCenterExtent(vec3{0.0f}, vec3{1.0f});
    {
        engine::geometry::Result result{};
        EXPECT_TRUE(engine::geometry::Intersects(r, box, &result));
        EXPECT_FLOAT_EQ(result.t_min, 1.0f);
        EXPECT_FLOAT_EQ(result.t_max, 3.0f);
    }

    const engine::geometry::Sphere s{vec3{0.0f}, 1.0f};
    {
        engine::geometry::Result result{};
        EXPECT_TRUE(engine::geometry::Intersects(r, s, &result));
        EXPECT_FLOAT_EQ(result.t, 1.0f);
    }
}

TEST(Segment, LengthAndInterpolation)
{
    const engine::geometry::Segment s{vec3{0.0f, 0.0f, 0.0f}, vec3{3.0f, 4.0f, 0.0f}};
    EXPECT_FLOAT_EQ(engine::geometry::Length(s), 5.0f);
    const vec3 midpoint{1.5f, 2.0f, 0.0f};
    expect_vec3_eq(engine::geometry::PointAt(s, 0.5f), midpoint);
}

TEST(Segment, ClosestPoint)
{
    const engine::geometry::Segment s{vec3{0.0f, 0.0f, 0.0f}, vec3{2.0f, 0.0f, 0.0f}};
    double t = 0.0;
    const vec3 point{1.0f, 1.0f, 0.0f};
    const vec3 expected{1.0f, 0.0f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(s, point, t), expected);
    EXPECT_EQ(t, 0.5);
    EXPECT_EQ(engine::geometry::SquaredDistance(s, point), 1.0);

    const vec3 outside{-1.0f, 0.0f, 0.0f};
    expect_vec3_eq(engine::geometry::ClosestPoint(s, outside, t), s.start);
    EXPECT_EQ(t, 0.0);
}

TEST(Line, Projection)
{
    const engine::geometry::Line l{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    const vec3 projected = engine::geometry::ClosestPoint(l, vec3{2.0f, 3.0f, -1.0f});
    expect_vec3_eq(projected, vec3{0.0f, 3.0f, 0.0f});

    const engine::geometry::Line degenerate_line{vec3{1.0f, 2.0f, 3.0f}, vec3{0.0f, 0.0f, 0.0f}};
    expect_vec3_eq(engine::geometry::ClosestPoint(degenerate_line, vec3{5.0f, -1.0f, 2.0f}), degenerate_line.point);
}

TEST(Triangle, DerivedQuantities)
{
    const Triangle triangle{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};

    const vec3 expected_normal{0.0f, 0.0f, 1.0f};
    expect_vec3_eq(engine::geometry::Normal(triangle), expected_normal);
    expect_vec3_eq(engine::geometry::UnitNormal(triangle), expected_normal);
    EXPECT_FLOAT_EQ(static_cast<float>(engine::geometry::Area(triangle)), 0.5f);
    expect_vec3_eq(engine::geometry::Centroid(triangle), vec3{1.0f / 3.0f, 1.0f / 3.0f, 0.0f});
}

TEST(Triangle, ContainsAndBarycentric)
{
    const Triangle triangle{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};
    const vec3 interior{0.25f, 0.25f, 0.0f};
    const vec3 exterior{0.5f, 0.5f, 0.2f};

    EXPECT_TRUE(engine::geometry::Contains(triangle, interior));
    EXPECT_FALSE(engine::geometry::Contains(triangle, exterior));

    const Triangle smaller{vec3{0.1f, 0.1f, 0.0f}, vec3{0.3f, 0.1f, 0.0f}, vec3{0.1f, 0.3f, 0.0f}};
    EXPECT_TRUE(engine::geometry::Contains(triangle, smaller));

    const vec3 bary = engine::geometry::ToBarycentricCoords(triangle, engine::geometry::Normal(triangle), interior);
    expect_vec3_eq(bary, vec3{0.5f, 0.25f, 0.25f});
    expect_vec3_eq(engine::geometry::FromBarycentricCoords(triangle, bary), interior);
}

TEST(Triangle, IntersectionsWithShapes)
{
    const Triangle triangle{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 1.0f, 0.0f}};

    const Aabb intersecting_box = engine::geometry::MakeAabbFromCenterExtent(
        vec3{0.25f, 0.25f, 0.0f}, vec3{0.3f, 0.3f, 0.1f});
    const Aabb far_box = engine::geometry::MakeAabbFromCenterExtent(vec3{0.0f, 0.0f, 1.5f}, vec3{0.2f});
    EXPECT_TRUE(engine::geometry::Intersects(triangle, intersecting_box));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, far_box));

    const engine::geometry::Obb obb = engine::geometry::MakeObbFromCenterHalfSizes(
        vec3{0.25f, 0.25f, 0.0f}, vec3{0.4f, 0.2f, 0.1f}, quat::Identity());
    EXPECT_TRUE(engine::geometry::Intersects(triangle, obb));

    const engine::geometry::Sphere sphere{vec3{0.2f, 0.2f, 0.0f}, 0.1f};
    const engine::geometry::Sphere far_sphere{vec3{0.0f, 0.0f, 1.0f}, 0.2f};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, sphere));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, far_sphere));

    const engine::geometry::Cylinder cylinder{vec3{0.3f, 0.3f, 0.0f}, vec3{0.0f, 0.0f, 1.0f}, 0.25f, 1.0f};
    const engine::geometry::Cylinder far_cylinder{vec3{0.0f, 0.0f, 2.0f}, vec3{0.0f, 0.0f, 1.0f}, 0.2f, 0.5f};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, cylinder));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, far_cylinder));

    const auto orientation = engine::math::quat::Identity();
    const engine::geometry::Ellipsoid ellipsoid{vec3{0.3f, 0.3f, 0.0f}, vec3{0.4f, 0.2f, 0.2f}, orientation};
    const engine::geometry::Ellipsoid far_ellipsoid{vec3{0.0f, 0.0f, 1.2f}, vec3{0.2f, 0.2f, 0.2f}, orientation};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, ellipsoid));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, far_ellipsoid));

    const engine::geometry::Line line{vec3{0.25f, 0.25f, -1.0f}, vec3{0.0f, 0.0f, 1.0f}};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, line));

    const engine::geometry::Plane plane{vec3{0.0f, 0.0f, 1.0f}, 0.0f};
    const engine::geometry::Plane far_plane{vec3{0.0f, 0.0f, 1.0f}, -1.0f};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, plane));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, far_plane));

    const engine::geometry::Ray ray{vec3{0.25f, 0.25f, 1.0f}, vec3{0.0f, 0.0f, -1.0f}};
    const engine::geometry::Ray miss_ray{vec3{0.25f, 0.25f, 1.0f}, vec3{0.0f, 0.0f, 1.0f}};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, ray));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, miss_ray));

    const engine::geometry::Segment segment{vec3{0.25f, 0.25f, 1.0f}, vec3{0.25f, 0.25f, -1.0f}};
    const engine::geometry::Segment miss_segment{vec3{2.0f, 2.0f, 0.0f}, vec3{2.0f, 2.0f, 1.0f}};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, segment));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, miss_segment));

    const Triangle other{vec3{0.25f, 0.25f, 0.0f}, vec3{0.75f, 0.25f, 0.0f}, vec3{0.25f, 0.75f, 0.0f}};
    const Triangle far_triangle{vec3{0.0f, 0.0f, 1.0f}, vec3{0.5f, 0.0f, 1.0f}, vec3{0.0f, 0.5f, 1.0f}};
    EXPECT_TRUE(engine::geometry::Intersects(triangle, other));
    EXPECT_TRUE(!engine::geometry::Intersects(triangle, far_triangle));
}

TEST(Ellipsoid, ContainsAndVolume)
{
    const float angle = std::numbers::pi_v<float> * 0.5f;
    const mat3 orientation{
        std::cos(angle), -std::sin(angle), 0.0f,
        std::sin(angle), std::cos(angle), 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    const quat orientation_quat = engine::math::from_rotation_matrix(orientation);

    const engine::geometry::Ellipsoid e{
        vec3{0.0f, 0.0f, 0.0f},
        vec3{2.0f, 1.0f, 0.5f},
        orientation_quat,
    };

    EXPECT_FLOAT_EQ(engine::geometry::Volume(e), (4.0f / 3.0f) * std::numbers::pi_v<float>);

    const vec3 inside = e.center +
        vec3(engine::math::utils::to_rotation_matrix(e.orientation) * vec3{1.0f, 0.0f, 0.0f});
    const vec3 outside_point{3.0f, 0.0f, 0.0f};
    EXPECT_TRUE(engine::geometry::Contains(e, inside));
    EXPECT_TRUE(!engine::geometry::Contains(e, outside_point));
}

TEST(Triangle, AreaNormalAndCentroid)
{
    const engine::geometry::Triangle t{vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{0.0f, 2.0f, 0.0f}};

    expect_vec3_eq(engine::geometry::Normal(t), vec3{0.0f, 0.0f, 2.0f});
    expect_vec3_eq(engine::geometry::UnitNormal(t), vec3{0.0f, 0.0f, 1.0f});
    EXPECT_FLOAT_EQ(engine::geometry::Area(t), 1.0f);
    expect_vec3_eq(engine::geometry::Centroid(t), vec3{1.0f / 3.0f, 2.0f / 3.0f, 0.0f});
}

TEST(Cylinder, AxisDerivedValuesAndContainment)
{
    const engine::geometry::Cylinder c{vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f, 0.0f, 2.0f}, 1.0f, 2.0f};

    expect_vec3_eq(engine::geometry::AxisDirection(c), vec3{0.0f, 0.0f, 1.0f});
    expect_vec3_eq(engine::geometry::TopCenter(c), vec3{0.0f, 0.0f, 2.0f});
    expect_vec3_eq(engine::geometry::BottomCenter(c), vec3{0.0f, 0.0f, -2.0f});

    EXPECT_FLOAT_EQ(engine::geometry::Volume(c), 4.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::LateralSurfaceArea(c), 8.0f * std::numbers::pi_v<float>);
    EXPECT_FLOAT_EQ(engine::geometry::SurfaceArea(c), 10.0f * std::numbers::pi_v<float>);

    const vec3 radial_inside{0.5f, 0.0f, 1.0f};
    const vec3 radial_outside{1.1f, 0.0f, 0.0f};
    const vec3 origin{0.0f, 0.0f, 0.0f};
    EXPECT_TRUE(engine::geometry::Contains(c, radial_inside));
    EXPECT_TRUE(!engine::geometry::Contains(c, radial_outside));
    EXPECT_TRUE(!engine::geometry::Contains(engine::geometry::Cylinder{origin, vec3{0.0f}, 1.0f, 1.0f}, origin));
}

TEST(Cylinder, IntersectsSphere)
{
    const engine::geometry::Cylinder c{vec3{0.0f}, vec3{0.0f, 0.0f, 4.0f}, 1.0f, 2.0f};
    const engine::geometry::Sphere touching{vec3{0.0f, 0.0f, 3.0f}, 1.0f};
    const engine::geometry::Sphere separate{vec3{5.0f, 0.0f, 0.0f}, 1.0f};
    EXPECT_TRUE(engine::geometry::Intersects(c, touching));
    EXPECT_TRUE(!engine::geometry::Intersects(c, separate));
}


TEST(CylinderGeometry, ClosestPointInsideCylinder)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 2.0f, 3.0f};
    const vec3 inside{1.0f, 0, 1.0f};

    const vec3 closest = ClosestPoint(cyl, inside);

    // Point inside should return itself
    EXPECT_FLOAT_EQ(closest[0], inside[0]);
    EXPECT_FLOAT_EQ(closest[1], inside[1]);
    EXPECT_FLOAT_EQ(closest[2], inside[2]);

    EXPECT_EQ(SquaredDistance(cyl, inside), 0.0);
}

TEST(CylinderGeometry, ClosestPointOnLateralSurface)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
    const vec3 outside{3.0f, 0, 0};

    const vec3 closest = ClosestPoint(cyl, outside);

    // Should be on lateral surface at x=1, z=0
    EXPECT_FLOAT_EQ(closest[0], 1.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);
    EXPECT_FLOAT_EQ(closest[2], 0.0f);

    const double dist_sq = SquaredDistance(cyl, outside);
    EXPECT_EQ(dist_sq, 4.0); // (3-1)^2 = 4
}

TEST(CylinderGeometry, ClosestPointOnTopCap)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
    const vec3 above{0, 0, 5.0f};

    const vec3 closest = ClosestPoint(cyl, above);

    // Should be at center of top cap
    EXPECT_FLOAT_EQ(closest[0], 0.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);
    EXPECT_FLOAT_EQ(closest[2], 2.0f);

    const double dist_sq = SquaredDistance(cyl, above);
    EXPECT_EQ(dist_sq, 9.0); // (5-2)^2 = 9
}

TEST(CylinderGeometry, ClosestPointOnBottomCap)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
    const vec3 below{0, 0, -5.0f};

    const vec3 closest = ClosestPoint(cyl, below);

    // Should be at center of bottom cap
    EXPECT_FLOAT_EQ(closest[0], 0.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);
    EXPECT_FLOAT_EQ(closest[2], -2.0f);

    const double dist_sq = SquaredDistance(cyl, below);
    EXPECT_EQ(dist_sq, 9.0); // (-5-(-2))^2 = 9
}

TEST(CylinderGeometry, ClosestPointOnCapEdge)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
    const vec3 diagonal{2.0f, 0, 3.0f};

    const vec3 closest = ClosestPoint(cyl, diagonal);

    // Should be on edge of top cap
    EXPECT_FLOAT_EQ(closest[0], 1.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);
    EXPECT_FLOAT_EQ(closest[2], 2.0f);

    const double dist_sq = SquaredDistance(cyl, diagonal);
    EXPECT_EQ(dist_sq, 2.0); // (2-1)^2 + (3-2)^2 = 2
}

TEST(CylinderGeometry, ClosestPointOnSurface)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.0f, 2.0f};
    const vec3 on_surface{1.0f, 0, 0};

    const vec3 closest = ClosestPoint(cyl, on_surface);

    // Point already on surface
    EXPECT_FLOAT_EQ(closest[0], 1.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);
    EXPECT_FLOAT_EQ(closest[2], 0.0f);

    EXPECT_TRUE(engine::math::utils::nearly_equal(SquaredDistance(cyl, on_surface), 0.0, 1e-6));
}

TEST(CylinderGeometry, ClosestPointWithOffset)
{
    const engine::geometry::Cylinder cyl{{5, 3, 2}, {0, 1, 0}, 2.0f, 3.0f};
    const vec3 point{10, 3, 2};

    const vec3 closest = ClosestPoint(cyl, point);

    // Should be on lateral surface at x=7, y=3, z=2
    EXPECT_FLOAT_EQ(closest[0], 7.0f);
    EXPECT_FLOAT_EQ(closest[1], 3.0f);
    EXPECT_FLOAT_EQ(closest[2], 2.0f);

    const double dist_sq = SquaredDistance(cyl, point);
    EXPECT_EQ(dist_sq, 9.0); // (10-7)^2 = 9
}

TEST(CylinderGeometry, ClosestPointNonAxisAligned)
{
    // Cylinder along x-axis
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {1, 0, 0}, 1.0f, 2.0f};
    const vec3 point{0, 3, 0};

    const vec3 closest = ClosestPoint(cyl, point);

    // Should be on lateral surface
    EXPECT_FLOAT_EQ(closest[0], 0.0f);
    EXPECT_FLOAT_EQ(closest[1], 1.0f);
    EXPECT_FLOAT_EQ(closest[2], 0.0f);

    const double dist_sq = SquaredDistance(cyl, point);
    EXPECT_EQ(dist_sq, 4.0); // (3-1)^2 = 4
}

TEST(CylinderGeometry, SquaredDistanceConsistency)
{
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 1.5f, 3.0f};

    // Test multiple points
    const std::vector<vec3> test_points = {
        {0, 0, 0}, // center
        {2, 2, 0}, // outside lateral
        {0, 0, 5}, // above
        {1, 1, 4}, // diagonal above
        {0.5f, 0.5f, 1} // inside
    };

    for (const auto& point : test_points)
    {
        const vec3 closest = ClosestPoint(cyl, point);
        const double dist_sq = SquaredDistance(cyl, point);
        const double expected_dist_sq = length_squared(point - closest);

        EXPECT_TRUE(engine::math::utils::nearly_equal(dist_sq, expected_dist_sq, 1e-5));
    }
}

TEST(CylinderGeometry, EdgeCaseZeroRadius)
{
    // Degenerate cylinder (line segment)
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 0.0f, 2.0f};
    const vec3 point{1, 0, 0};

    const vec3 closest = ClosestPoint(cyl, point);

    // Should be on the axis
    EXPECT_FLOAT_EQ(closest[0], 0.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);

    const double dist_sq = SquaredDistance(cyl, point);
    EXPECT_EQ(dist_sq, 1.0);
}

TEST(CylinderGeometry, EdgeCaseZeroHeight)
{
    // Flat disk
    const engine::geometry::Cylinder cyl{{0, 0, 0}, {0, 0, 1}, 2.0f, 0.0f};
    const vec3 above{1, 0, 3};

    const vec3 closest = ClosestPoint(cyl, above);

    // Should be on the disk at z=0
    EXPECT_FLOAT_EQ(closest[0], 1.0f);
    EXPECT_FLOAT_EQ(closest[1], 0.0f);
    EXPECT_FLOAT_EQ(closest[2], 0.0f);

    const double dist_sq = SquaredDistance(cyl, above);
    EXPECT_EQ(dist_sq, 9.0); // z-distance squared
}
