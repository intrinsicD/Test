#include <gtest/gtest.h>

#include "engine/geometry/shapes/frustum.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

#include <cmath>

using namespace engine::geometry;
using namespace engine::math;

namespace {
    constexpr float kEpsilon = 1e-5f;

    bool ApproxEqual(float a, float b, float epsilon = kEpsilon) {
        return std::abs(a - b) < epsilon;
    }

    bool ApproxEqual(const vec3& a, const vec3& b, float epsilon = kEpsilon) {
        return ApproxEqual(a[0], b[0], epsilon) &&
               ApproxEqual(a[1], b[1], epsilon) &&
               ApproxEqual(a[2], b[2], epsilon);
    }
}

TEST(FrustumTest, ExtractFrustumFromIdentity) {
    // Simple orthographic projection as a basic test
    // Create a simple orthographic matrix: maps [-1,1] cube to clip space
    mat4 ortho{};
    ortho[0][0] = 1.0f;  // x scale
    ortho[1][1] = 1.0f;  // y scale
    ortho[2][2] = -1.0f; // z scale (flip for right-handed)
    ortho[3][3] = 1.0f;  // w

    const Frustum frustum = ExtractFrustum(ortho);

    // All planes should be normalized
    for (const auto& plane : frustum.planes) {
        const float len = length(plane.normal);
        EXPECT_NEAR(len, 1.0f, kEpsilon) << "Plane normal should be normalized";
    }
}

TEST(FrustumTest, ExtractFrustumFromPerspective) {
    // Create a simple perspective projection matrix
    // Using manual construction for clarity
    const float fov = 3.14159f / 4.0f; // 45 degrees
    const float aspect = 16.0f / 9.0f;
    const float near = 0.1f;
    const float far = 100.0f;

    const float tan_half_fov = std::tan(fov / 2.0f);
    const float f = 1.0f / tan_half_fov;

    mat4 proj;
    proj[0][0] = f / aspect;
    proj[1][1] = f;
    proj[2][2] = -(far + near) / (far - near);
    proj[2][3] = -1.0f;
    proj[3][2] = -(2.0f * far * near) / (far - near);
    proj[3][3] = 0.0f;

    const Frustum frustum = ExtractFrustum(proj);

    // All planes should be normalized
    for (size_t i = 0; i < 6; ++i) {
        const float len = length(frustum.planes[i].normal);
        EXPECT_NEAR(len, 1.0f, kEpsilon) << "Plane " << i << " normal should be normalized";
    }

    // Near and far planes should roughly point in opposite Z directions
    EXPECT_LT(frustum.planes[Frustum::kNear].normal[2], 0.0f) << "Near plane should point backward";
    EXPECT_GT(frustum.planes[Frustum::kFar].normal[2], 0.0f) << "Far plane should point forward";
}

TEST(FrustumTest, ExtractFrustumFromOrthographic) {
    // Create an orthographic projection matrix
    const float left = -10.0f, right = 10.0f;
    const float bottom = -10.0f, top = 10.0f;
    const float near = 0.1f, far = 100.0f;

    mat4 ortho;
    ortho[0][0] = 2.0f / (right - left);
    ortho[1][1] = 2.0f / (top - bottom);
    ortho[2][2] = -2.0f / (far - near);
    ortho[3][0] = -(right + left) / (right - left);
    ortho[3][1] = -(top + bottom) / (top - bottom);
    ortho[3][2] = -(far + near) / (far - near);
    ortho[3][3] = 1.0f;

    const Frustum frustum = ExtractFrustum(ortho);

    // All planes should be normalized
    for (const auto& plane : frustum.planes) {
        const float len = length(plane.normal);
        EXPECT_NEAR(len, 1.0f, kEpsilon) << "Plane normal should be normalized";
    }
}

TEST(FrustumTest, PointInsideFrustum) {
    // Simple frustum: identity matrix
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Origin should be inside a typical frustum
    const vec3 origin{0.0f, 0.0f, 0.0f};
    EXPECT_TRUE(Intersects(frustum, origin)) << "Origin should be inside frustum";
}

TEST(FrustumTest, PointOutsideFrustum) {
    // Create a frustum with known bounds
    const float fov = 3.14159f / 4.0f;
    const float aspect = 1.0f;
    const float near = 1.0f;
    const float far = 10.0f;

    const float tan_half_fov = std::tan(fov / 2.0f);
    const float f = 1.0f / tan_half_fov;

    mat4 proj;
    proj[0][0] = f / aspect;
    proj[1][1] = f;
    proj[2][2] = -(far + near) / (far - near);
    proj[2][3] = -1.0f;
    proj[3][2] = -(2.0f * far * near) / (far - near);
    proj[3][3] = 0.0f;

    const Frustum frustum = ExtractFrustum(proj);

    // Point far outside should not be inside
    const vec3 far_point{1000.0f, 1000.0f, 1000.0f};
    EXPECT_FALSE(Intersects(frustum, far_point)) << "Far point should be outside frustum";
}

TEST(FrustumTest, AabbFullyInsideFrustum) {
    // Simple frustum
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Small AABB at origin
    const Aabb small_box{{-0.1f, -0.1f, -0.1f}, {0.1f, 0.1f, 0.1f}};
    EXPECT_TRUE(Intersects(frustum, small_box)) << "Small box at origin should intersect frustum";
}

TEST(FrustumTest, AabbFullyOutsideFrustum) {
    // Create perspective frustum
    const float fov = 3.14159f / 4.0f;
    const float aspect = 1.0f;
    const float near = 1.0f;
    const float far = 10.0f;

    const float tan_half_fov = std::tan(fov / 2.0f);
    const float f = 1.0f / tan_half_fov;

    mat4 proj;
    proj[0][0] = f / aspect;
    proj[1][1] = f;
    proj[2][2] = -(far + near) / (far - near);
    proj[2][3] = -1.0f;
    proj[3][2] = -(2.0f * far * near) / (far - near);
    proj[3][3] = 0.0f;

    const Frustum frustum = ExtractFrustum(proj);

    // AABB far outside
    const Aabb far_box{{1000.0f, 1000.0f, 1000.0f}, {1001.0f, 1001.0f, 1001.0f}};
    EXPECT_FALSE(Intersects(frustum, far_box)) << "Far box should be outside frustum";
}

TEST(FrustumTest, AabbPartiallyIntersectingFrustum) {
    // Simple frustum
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Large AABB that likely intersects frustum boundaries
    const Aabb large_box{{-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f}};
    EXPECT_TRUE(Intersects(frustum, large_box)) << "Large box should intersect frustum";
}

TEST(FrustumTest, SphereFullyInsideFrustum) {
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Small sphere at origin
    const Sphere small_sphere{{0.0f, 0.0f, 0.0f}, 0.1f};
    EXPECT_TRUE(Intersects(frustum, small_sphere)) << "Small sphere at origin should intersect frustum";
}

TEST(FrustumTest, SphereFullyOutsideFrustum) {
    // Create perspective frustum
    const float fov = 3.14159f / 4.0f;
    const float aspect = 1.0f;
    const float near = 1.0f;
    const float far = 10.0f;

    const float tan_half_fov = std::tan(fov / 2.0f);
    const float f = 1.0f / tan_half_fov;

    mat4 proj;
    proj[0][0] = f / aspect;
    proj[1][1] = f;
    proj[2][2] = -(far + near) / (far - near);
    proj[2][3] = -1.0f;
    proj[3][2] = -(2.0f * far * near) / (far - near);
    proj[3][3] = 0.0f;

    const Frustum frustum = ExtractFrustum(proj);

    // Sphere far outside
    const Sphere far_sphere{{1000.0f, 1000.0f, 1000.0f}, 1.0f};
    EXPECT_FALSE(Intersects(frustum, far_sphere)) << "Far sphere should be outside frustum";
}

TEST(FrustumTest, SphereTouchingFrustumPlane) {
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Sphere with large radius should likely touch frustum
    const Sphere large_sphere{{0.0f, 0.0f, 0.0f}, 10.0f};
    EXPECT_TRUE(Intersects(frustum, large_sphere)) << "Large sphere should intersect frustum";
}

TEST(FrustumTest, SymmetricIntersectionAabb) {
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);
    const Aabb box{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};

    // Test symmetric overloads
    EXPECT_EQ(Intersects(frustum, box), Intersects(box, frustum))
        << "Symmetric AABB-Frustum intersection should match";
}

TEST(FrustumTest, SymmetricIntersectionSphere) {
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);
    const Sphere sphere{{0.0f, 0.0f, 0.0f}, 1.0f};

    // Test symmetric overloads
    EXPECT_EQ(Intersects(frustum, sphere), Intersects(sphere, frustum))
        << "Symmetric Sphere-Frustum intersection should match";
}

TEST(FrustumTest, GetCornersReturnsEightPoints) {
    // Simple orthographic projection
    mat4 ortho{};
    ortho[0][0] = 1.0f;
    ortho[1][1] = 1.0f;
    ortho[2][2] = -1.0f;
    ortho[3][3] = 1.0f;

    const Frustum frustum = ExtractFrustum(ortho);

    const auto corners = GetCorners(frustum);
    EXPECT_EQ(corners.size(), 8u) << "Frustum should have 8 corners";

    // Check that corners are not all at origin (would indicate failure)
    bool has_non_zero = false;
    for (const auto& corner : corners) {
        if (length(corner) > kEpsilon) {
            has_non_zero = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_zero) << "At least some corners should be non-zero";
}

TEST(FrustumTest, DegenerateAabbZeroSize) {
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Zero-size AABB (point)
    const Aabb point_box{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

    // Should behave like point containment test
    const bool intersects = Intersects(frustum, point_box);
    const bool point_inside = Intersects(frustum, vec3{0.0f, 0.0f, 0.0f});
    EXPECT_EQ(intersects, point_inside) << "Zero-size AABB should behave like point test";
}

TEST(FrustumTest, DegenerateSphereZeroRadius) {
    const mat4 identity{};
    const Frustum frustum = ExtractFrustum(identity);

    // Zero-radius sphere (point)
    const Sphere point_sphere{{0.0f, 0.0f, 0.0f}, 0.0f};

    // Should behave like point containment test
    const bool intersects = Intersects(frustum, point_sphere);
    const bool point_inside = Intersects(frustum, vec3{0.0f, 0.0f, 0.0f});
    EXPECT_EQ(intersects, point_inside) << "Zero-radius sphere should behave like point test";
}

