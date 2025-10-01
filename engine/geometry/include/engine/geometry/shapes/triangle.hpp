#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Triangle {
        math::vec3 a;
        math::vec3 b;
        math::vec3 c;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Normal(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 UnitNormal(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Area(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Centroid(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Triangle &triangle, const math::vec3 &point) noexcept;
} // namespace engine::geometry
