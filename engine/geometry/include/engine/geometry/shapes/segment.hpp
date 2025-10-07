#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Segment {
        math::vec3 start;
        math::vec3 end;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Direction(const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Length(const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 PointAt(const Segment &segment, float t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Segment &segment,
                                                              const math::vec3 &point,
                                                              double &t_result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Segment &segment,
                                                             const math::vec3 &point) noexcept;

    ENGINE_GEOMETRY_API void Random(Segment &segment, RandomEngine &rng) noexcept;

    ENGINE_GEOMETRY_API void Random(Segment &segment) noexcept;
} // namespace engine::geometry
