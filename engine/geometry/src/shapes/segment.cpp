#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

#include "engine/geometry/shapes/plane.hpp"

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

    bool Contains(const Segment &outer, const math::vec3 &inner,
                  float epsilon) noexcept {
        //TODO
    }

    bool Contains(const Segment &outer, const Segment &inner,
                  float epsilon ) noexcept {
        //TODO
    }

    //------------------------------------------------------------------------------------------------------------------

    bool Intersects(const Segment &segment,
                    const Aabb &aabb,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Cylinder &cylinder,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Ellipsoid &ellipsoid,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &outer,
                    const Line &inner,
                    Segment1DHit *result,
                    float epsilon) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Obb &obb,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Plane &p,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Ray &ray,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Segment &other,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Sphere &sphere,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    bool Intersects(const Segment &segment,
                    const Triangle &trianle,
                    Segment1DHit *result) noexcept {
        //TODO
    }

    math::vec3 ClosestPoint(const Segment &segment,
                            const math::vec3 &point,
                            double &t_result) noexcept {
        //TODO
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
