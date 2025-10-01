#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

namespace engine::geometry {

math::vec3 direction(const Segment& s) noexcept {
    return s.end - s.start;
}

float length(const Segment& s) noexcept {
    return math::length(direction(s));
}

math::vec3 point_at(const Segment& s, float t) noexcept {
    return s.start + direction(s) * t;
}

bool Intersects(const Segment& s, const Plane& p, float& out_t) noexcept {
    return Intersects(p, s, out_t);
}

}  // namespace engine::geometry

