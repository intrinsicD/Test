#include "engine/geometry/shapes/line.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

namespace engine::geometry {

math::vec3 point_at(const line& l, float t) noexcept {
    return l.point + l.direction * t;
}

math::vec3 project_point(const line& l, const math::vec3& point) noexcept {
    const float denom = math::length_squared(l.direction);
    if (denom == 0.0f) {
        return l.point;
    }
    const math::vec3 offset = point - l.point;
    const float t = math::dot(offset, l.direction) / denom;
    return point_at(l, t);
}

bool intersects(const line& l, const plane& p, float& out_t) noexcept {
    return intersects(p, l, out_t);
}

}  // namespace engine::geometry

