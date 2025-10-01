#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct Plane;

    struct ENGINE_GEOMETRY_API Segment {
        math::vec3 start;
        math::vec3 end;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Direction(const Segment &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Length(const Segment &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 PointAt(const Segment &s, float t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &s, const Plane &p, float &out_t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Segment &segment, const math::vec3 &point) noexcept;
} // namespace engine::geometry
