#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/geometry/shapes/segment.hpp"
#include "engine/geometry/shapes/triangle.hpp"

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
        //TODO
    }

    bool Intersects(const Sphere &sphere, const Ray &ray) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &sphere, const Plane &plane) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &sphere, const Ellipsoid &ellipsoid) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &sphere, const Segment &segment) noexcept {
        //TODO
    }

    bool Intersects(const Sphere &spherephere, const Triangle &triangle) noexcept {
        //TODO
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
            max_distance_sq = std::max(max_distance_sq, dist_sq);
        }
        return Sphere{box.center, std::sqrt(max_distance_sq)};
    }


    Sphere BoundingSphere(const Segment &segment) noexcept {
        return {(segment.start + segment.end) * 0.5f, math::length(segment.end - segment.start) * 0.5f};
    }

    Sphere BoundingSphere(const Cylinder &cylinder) noexcept {
        //TODO
    }

    Sphere BoundingSphere(const Ellipsoid &ellipsoid) noexcept {
        //TODO
    }

    Sphere BoundingSphere(const Triangle &triangle) noexcept {
        //TODO
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
        const float dist = std::sqrt(dist_sq);
        const float new_radius = (sphere.radius + dist) * 0.5f;
        const float k = (new_radius - sphere.radius) / dist;
        sphere.center += offset * k;
        sphere.radius = new_radius;
    }

    math::vec3 ClosestPoint(const Sphere &sphere, const math::vec3 &point) noexcept {
        //TODO
    }

    double SquaredDistance(const Sphere &sphere, const math::vec3 &point) noexcept {
        //TODO
    }
} // namespace engine::geometry
