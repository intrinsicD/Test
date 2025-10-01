#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

namespace engine::geometry {

math::vec3 Direction(const Segment& s) noexcept {
    return s.end - s.start;
}

float Length(const Segment& s) noexcept {
    return math::length(Direction(s));
}

math::vec3 PointAt(const Segment& s, float t) noexcept {
    return s.start + Direction(s) * t;
}

bool Intersects(const Segment& s, const Plane& p, float& out_t) noexcept {
    return Intersects(p, s, out_t);
}

}  // namespace engine::geometry

