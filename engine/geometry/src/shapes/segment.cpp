#include "engine/geometry/shapes/segment.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/math/utils.hpp"

namespace engine::geometry {
    math::vec3 Direction(const Segment &segment) noexcept {
        return segment.end - segment.start;
    }

    float Length(const Segment &segment) noexcept {
        return math::length(Direction(segment));
    }

    math::vec3 PointAt(const Segment &segment, float t) noexcept {
        return segment.start + Direction(segment) * t;
    }

    math::vec3 ClosestPoint(const Segment &segment,
                            const math::vec3 &point,
                            double &t_result) noexcept {
        const math::vec3 direction = Direction(segment);
        const float length_sq = math::length_squared(direction);
        if (length_sq == 0.0f) {
            t_result = 0.0;
            return segment.start;
        }

        const math::vec3 offset = point - segment.start;
        const float t = math::dot(offset, direction) / length_sq;
        const float clamped_t = math::utils::clamp(t, 0.0f, 1.0f);
        t_result = static_cast<double>(clamped_t);
        return segment.start + direction * clamped_t;
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
