#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

namespace engine::geometry {

math::vec3 direction(const segment& s) noexcept {
    return s.end - s.start;
}

float length(const segment& s) noexcept {
    return math::length(direction(s));
}

math::vec3 point_at(const segment& s, float t) noexcept {
    return s.start + direction(s) * t;
}

bool intersects(const segment& s, const plane& p, float& out_t) noexcept {
    return intersects(p, s, out_t);
}

}  // namespace engine::geometry

