#include "engine/geometry/shapes/plane.hpp"

#include <cmath>

#include "engine/geometry/shapes/line.hpp"
#include "engine/geometry/shapes/ray.hpp"
#include "engine/geometry/shapes/segment.hpp"

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float epsilon() noexcept { return 1e-6f; }

}  // namespace

float SignedDistance(const Plane& p, const math::vec3& point) noexcept {
    return math::dot(p.normal, point) + p.distance;
}

math::vec3 ClosestPoint(const Plane& p, const math::vec3& point) noexcept {
    const float denom = math::length_squared(p.normal);
    if (denom == 0.0f) {
        return point;
    }
    const float dist = SignedDistance(p, point);
    return point - p.normal * (dist / denom);
}

bool Contains(const Plane& p, const math::vec3& point, float epsilon_value) noexcept {
    return std::fabs(SignedDistance(p, point)) <= epsilon_value;
}

bool Intersects(const Plane& p, const Ray& r, float& out_t) noexcept {
    const float denom = math::dot(p.normal, r.direction);
    if (std::fabs(denom) < epsilon()) {
        return false;
    }

    const float numer = -SignedDistance(p, r.origin);
    const float t = numer / denom;
    if (t < 0.0f) {
        return false;
    }

    out_t = t;
    return true;
}

bool Intersects(const Plane& p, const Line& l, float& out_t) noexcept {
    const float denom = math::dot(p.normal, l.direction);
    if (std::fabs(denom) < epsilon()) {
        return false;
    }

    const float numer = -SignedDistance(p, l.point);
    out_t = numer / denom;
    return true;
}

bool Intersects(const Plane& p, const Segment& s, float& out_t) noexcept {
    const Ray r{s.start, Direction(s)};
    if (!Intersects(p, r, out_t)) {
        return false;
    }
    if (out_t < 0.0f || out_t > 1.0f) {
        return false;
    }
    return true;
}

}  // namespace engine::geometry

