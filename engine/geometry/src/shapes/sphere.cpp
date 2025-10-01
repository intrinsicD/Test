#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/geometry/shapes/segment.hpp"
#include "engine/geometry/shapes/triangle.hpp"
#include "engine/geometry/shapes/line.hpp"
#include "engine/geometry/shapes/ray.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/geometry/shapes/ellipsoid.hpp"

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

    bool Contains(const Sphere &s, const math::vec3 &point) noexcept {
        const math::vec3 offset = point - s.center;
        return math::length_squared(offset) <= s.radius * s.radius;
    }

    bool Contains(const Sphere &outer, const Sphere &inner) noexcept {
        const math::vec3 offset = inner.center - outer.center;
        const float distance = math::length(offset);
        return distance + inner.radius <= outer.radius + std::numeric_limits<float>::epsilon();
    }

    bool Contains(const Sphere &outer, const Aabb &inner) noexcept {
        for (const auto &corner: GetCorners(inner)) {
            if (!Contains(outer, corner)) {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Sphere &outer, const Obb &inner) noexcept {
        for (const auto &corner: GetCorners(inner)) {
            if (!Contains(outer, corner)) {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Sphere &outer, const Cylinder &inner) noexcept {
        return Contains(outer, BoundingSphere(inner));
    }

    bool Contains(const Sphere &outer, const Ellipsoid &inner) noexcept {
        return Contains(outer, BoundingSphere(inner));
    }

    bool Contains(const Sphere &outer, const Segment &inner) noexcept {
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Sphere &outer, const Triangle &inner) noexcept {
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Intersects(const Sphere &lhs, const Sphere &rhs) noexcept {
        const math::vec3 offset = rhs.center - lhs.center;
        const float radii = lhs.radius + rhs.radius;
        return math::length_squared(offset) <= radii * radii;
    }

    bool Intersects(const Sphere &s, const Aabb &box) noexcept {
        return Intersects(box, s);
    }

    bool Intersects(const Sphere &s, const Obb &box) noexcept {
        return Intersects(box, s);
    }

    bool Intersects(const Sphere &s, const Cylinder &c) noexcept {
        return Intersects(c, s);
    }

    bool Intersects(const Sphere &sphere, const Line &line) noexcept {
        const double distance_sq = SquaredDistance(line, sphere.center);
        return distance_sq <= sphere.radius * sphere.radius;
    }

    bool Intersects(const Sphere &sphere, const Ray &ray) noexcept {
        float t = 0.0f;
        return Intersects(ray, sphere, t);
    }

    bool Intersects(const Sphere &sphere, const Plane &plane) noexcept {
        const float normal_length = math::length(plane.normal);
        if (normal_length == 0.0f) {
            return false;
        }
        const double distance = math::utils::abs(SignedDistance(plane, sphere.center)) / normal_length;
        return distance <= sphere.radius;
    }

    bool Intersects(const Sphere &sphere, const Ellipsoid &ellipsoid) noexcept {
        if (Contains(ellipsoid, sphere.center) || Contains(sphere, ellipsoid.center)) {
            return true;
        }
        return Intersects(sphere, BoundingSphere(ellipsoid));
    }

    bool Intersects(const Sphere &sphere, const Segment &segment) noexcept {
        const double distance_sq = SquaredDistance(segment, sphere.center);
        return distance_sq <= sphere.radius * sphere.radius;
    }

    bool Intersects(const Sphere &spherephere, const Triangle &triangle) noexcept {
        const double distance_sq = SquaredDistance(triangle, spherephere.center);
        return distance_sq <= static_cast<double>(spherephere.radius) * static_cast<double>(spherephere.radius);
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
        const math::vec3 offset = other.center - sphere.center;
        const math::vec3 other_point = other.center + offset * (other.radius / math::length(offset));
        Merge(sphere, other_point);
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
