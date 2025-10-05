#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Triangle {
        math::vec3 a;
        math::vec3 b;
        math::vec3 c;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Normal(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 UnitNormal(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Area(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Centroid(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Triangle &triangle,
                                                              const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Triangle &triangle,
                                                             const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ToBarycentricCoords(const Triangle &triangle,
                                                                 const math::vec3 &normal,
                                                                 const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 FromBarycentricCoords(const Triangle &triangle,
                                                                       const math::vec3 &bc) noexcept;
} // namespace engine::geometry
