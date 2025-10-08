#include "engine/geometry/shapes.hpp"
#include "engine/math/utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace engine::geometry {
    double SurfaceArea(const Sphere &s) noexcept {
        double radius = s.radius;
        return 4.0 * std::numbers::pi_v<double> * radius * radius;
    }

    double Volume(const Sphere &s) noexcept {
        double radius = s.radius;
        return 4.0 / 3.0 * std::numbers::pi_v<double> * radius * radius * radius;
    }

    Sphere BoundingSphere(const math::vec3 &point) noexcept {
        return Sphere{point, 0.0f};
    }

    Sphere BoundingSphere(const Aabb &box) noexcept {
        const math::vec3 c = Center(box);
        const math::vec3 ext = Extent(box);
        return Sphere{c, math::length(ext)};
    }

    Sphere BoundingSphere(const Obb &box) noexcept {
        float max_distance_sq = 0.0f;
        for (const auto &corner: GetCorners(box)) {
            const float dist_sq = math::length_squared(corner - box.center);
            max_distance_sq = math::utils::max(max_distance_sq, dist_sq);
        }
        return Sphere{box.center, math::utils::sqrt(max_distance_sq)};
    }

    Sphere BoundingSphere(const Segment &segment) noexcept {
        return {(segment.start + segment.end) * 0.5f, math::length(segment.end - segment.start) * 0.5f};
    }

    Sphere BoundingSphere(const Cylinder &cylinder) noexcept {
        const float radial_sq = cylinder.radius * cylinder.radius;
        const float height_sq = cylinder.half_height * cylinder.half_height;
        return Sphere{cylinder.center, math::utils::sqrt(radial_sq + height_sq)};
    }

    Sphere BoundingSphere(const Ellipsoid &ellipsoid) noexcept {
        const float max_radius = math::utils::max(ellipsoid.radii[0], ellipsoid.radii[1], ellipsoid.radii[2]);
        return Sphere{ellipsoid.center, max_radius};
    }

    Sphere BoundingSphere(const Triangle &triangle) noexcept {
        const math::vec3 a = triangle.a;
        const math::vec3 b = triangle.b;
        const math::vec3 c = triangle.c;

        const math::vec3 ab = b - a;
        const math::vec3 ac = c - a;
        const math::vec3 bc = c - b;

        const float ab_len_sq = math::length_squared(ab);
        const float ac_len_sq = math::length_squared(ac);
        const float bc_len_sq = math::length_squared(bc);

        auto sphere_from_segment = [](const math::vec3 &p0, const math::vec3 &p1) noexcept {
            return BoundingSphere(Segment{p0, p1});
        };

        const float side_a_sq = bc_len_sq;
        const float side_b_sq = ac_len_sq;
        const float side_c_sq = ab_len_sq;

        if (side_a_sq >= side_b_sq + side_c_sq) {
            return sphere_from_segment(b, c);
        }
        if (side_b_sq >= side_a_sq + side_c_sq) {
            return sphere_from_segment(a, c);
        }
        if (side_c_sq >= side_a_sq + side_b_sq) {
            return sphere_from_segment(a, b);
        }

        const math::vec3 normal = math::cross(ab, ac);
        const float normal_len_sq = math::length_squared(normal);
        if (normal_len_sq == 0.0f) {
            // Degenerate triangle; fall back to largest segment
            float max_len_sq = ab_len_sq;
            Sphere best = sphere_from_segment(a, b);
            if (ac_len_sq > max_len_sq) {
                max_len_sq = ac_len_sq;
                best = sphere_from_segment(a, c);
            }
            if (bc_len_sq > max_len_sq) {
                best = sphere_from_segment(b, c);
            }
            return best;
        }

        const float denom = 2.0f * normal_len_sq;
        const math::vec3 term1 = math::cross(normal, ab) * ac_len_sq;
        const math::vec3 term2 = math::cross(ac, normal) * ab_len_sq;
        const math::vec3 center = a + (term1 + term2) / denom;
        const float radius = math::length(center - a);
        return Sphere{center, radius};
    }


    Sphere Merge(const Sphere &l, const Sphere &r) noexcept {
        Sphere result = l;
        Merge(result, r);
        return result;
    }

    void Merge(Sphere &sphere, const Sphere &other) noexcept {
        if (sphere.radius < 0.0f) { // Handle uninitialized sphere
            sphere = other;
            return;
        }
        if (other.radius < 0.0f) {
            return;
        }

        const math::vec3 offset = other.center - sphere.center;
        const float dist_sq = math::length_squared(offset);
        const float radius_diff = sphere.radius - other.radius;

        // Check if one sphere is fully contained within the other
        if (radius_diff * radius_diff >= dist_sq) {
            // If r is inside l, do nothing.
            // If l is inside r, update l to be other.
            if (sphere.radius < other.radius) {
                sphere = other;
            }
            return;
        }

        // Spheres are external or intersecting, calculate new bounding sphere
        const float dist = math::utils::sqrt(dist_sq);
        const float new_radius = (dist + sphere.radius + other.radius) * 0.5f;

        // The new center is on the line connecting the old centers
        const math::vec3 direction = (dist > 0.0f) ? (offset / dist) : math::vec3{1.0f, 0.0f, 0.0f};
        sphere.center = other.center - direction * (new_radius - other.radius);
        sphere.radius = new_radius;
    }

    void Merge(Sphere &sphere, const math::vec3 &point) noexcept {
        const math::vec3 offset = point - sphere.center;
        const float dist_sq = math::length_squared(offset);
        if (dist_sq <= sphere.radius * sphere.radius) {
            return; // point is already inside the sphere
        }
        const float dist = math::utils::sqrt(dist_sq);
        const float new_radius = (sphere.radius + dist) * 0.5f;
        const float k = (new_radius - sphere.radius) / dist;
        sphere.center += offset * k;
        sphere.radius = new_radius;
    }

    math::vec3 ClosestPoint(const Sphere &sphere, const math::vec3 &point) noexcept {
        const math::vec3 offset = point - sphere.center;
        const float dist_sq = math::length_squared(offset);
        if (dist_sq <= sphere.radius * sphere.radius || dist_sq == 0.0f) {
            return point;
        }
        const float dist = math::utils::sqrt(dist_sq);
        const float scale = sphere.radius / dist;
        return sphere.center + offset * scale;
    }

    double SquaredDistance(const Sphere &sphere, const math::vec3 &point) noexcept {
        const math::vec3 offset = point - sphere.center;
        const double dist_sq = math::length_squared(offset);
        const double radius_sq = static_cast<double>(sphere.radius) * static_cast<double>(sphere.radius);
        if (dist_sq <= radius_sq) {
            return 0.0;
        }
        const double dist = math::utils::sqrt(dist_sq);
        const double radius = sphere.radius;
        const double delta = dist - radius;
        return delta * delta;
    }
} // namespace engine::geometry
