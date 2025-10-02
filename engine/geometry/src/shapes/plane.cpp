#include "engine/geometry/shapes/plane.hpp"

#include <cmath>

#include "engine/geometry/shapes/line.hpp"
#include "engine/geometry/shapes/ray.hpp"
#include "engine/geometry/shapes/segment.hpp"

namespace engine::geometry {
    namespace {
        [[nodiscard]] constexpr float epsilon() noexcept { return 1e-6f; }
    } // namespace

    float SignedDistance(const Plane &p, const math::vec3 &point) noexcept {
        return math::dot(p.normal, point) + p.distance;
    }

    math::vec3 ClosestPoint(const Plane &p, const math::vec3 &point) noexcept {
        const float denom = math::length_squared(p.normal);
        if (denom == 0.0f) {
            return point;
        }
        const float dist = SignedDistance(p, point);
        return point - p.normal * (dist / denom);
    }

    double SquaredDistance(const Plane &p, const math::vec3 &point) noexcept {
        const float dist = SignedDistance(p, point);
        return static_cast<double>(dist * dist) / static_cast<double>(math::length_squared(p.normal));
    }
} // namespace engine::geometry
