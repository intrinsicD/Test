#include "engine/geometry/shapes/ray.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/geometry/shapes.hpp"

namespace engine::geometry {
    math::vec3 PointAt(const Ray &ray, float t) noexcept {
        return ray.origin + ray.direction * t;
    }

    math::vec3 ClosestPoint(const Ray &ray, const math::vec3 &point) noexcept {
        const float denom = math::length_squared(ray.direction);
        if (denom == 0.0f) {
            return ray.origin;
        }

        const math::vec3 offset = point - ray.origin;
        float t = math::dot(offset, ray.direction) / denom;
        if (t < 0.0f) {
            t = 0.0f;
        }

        return PointAt(ray, t);
    }

    double SquaredDistance(const Ray &ray, const math::vec3 &point) noexcept {
        const math::vec3 closest = ClosestPoint(ray, point);
        const math::vec3 diff = point - closest;
        return static_cast<double>(math::dot(diff, diff));
    }
} // namespace engine::geometry
