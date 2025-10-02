#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Plane {
        math::vec3 normal;
        float distance;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API float SignedDistance(const Plane &p, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Plane &p, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Plane &p, const math::vec3 &point) noexcept;
} // namespace engine::geometry
