#include "engine/geometry/shapes/line.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

namespace engine::geometry {
    math::vec3 PointAt(const Line &l, float t) noexcept {
        return l.point + l.direction * t;
    }

    math::vec3 ClosestPoint(const Line &l, const math::vec3 &point) noexcept {
        const float denom = math::length_squared(l.direction);
        if (denom == 0.0f) {
            return l.point;
        }
        const math::vec3 offset = point - l.point;
        const float t = math::dot(offset, l.direction) / denom;
        return PointAt(l, t);
    }

    double SquaredDistance(const Line &line, const math::vec3 &point) noexcept {
        const float denom = math::length_squared(line.direction);
        if (denom == 0.0f) {
            return math::length_squared(point - line.point);
        }

        const float t = math::dot(point - line.point, line.direction) / denom;
        const math::vec3 closest = line.point + line.direction * t;
        return math::length_squared(point - closest);
    }
} // namespace engine::geometry
