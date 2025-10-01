#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

namespace engine::geometry {
    math::vec3 Direction(const Segment &s) noexcept {
        return s.end - s.start;
    }

    float Length(const Segment &s) noexcept {
        return math::length(Direction(s));
    }

    math::vec3 PointAt(const Segment &s, float t) noexcept {
        return s.start + Direction(s) * t;
    }

    bool Intersects(const Segment &s, const Plane &p, float &out_t) noexcept {
        return Intersects(p, s, out_t);
    }

    double SquaredDistance(const Segment &segment, const math::vec3 &point) noexcept {
        const math::vec3 direction = segment.end - segment.start;
        const float length_sq = math::length_squared(direction);
        if (length_sq == 0.0f) {
            return math::length_squared(point - segment.start);
        }

        const float t = math::dot(point - segment.start, direction) / length_sq;
        const float clamped_t = math::utils::clamp(t, 0.0f, 1.0f);
        const math::vec3 closest = segment.start + direction * clamped_t;
        return math::length_squared(point - closest);
    }
} // namespace engine::geometry
