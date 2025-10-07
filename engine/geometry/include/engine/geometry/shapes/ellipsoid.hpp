#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/quaternion.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Ellipsoid {
        math::vec3 center;
        math::vec3 radii;
        math::quat orientation;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API float Volume(const Ellipsoid &ellipsoid) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Ellipsoid &ellipsoid, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Ellipsoid &ellipsoid, const math::vec3 &point) noexcept;

    ENGINE_GEOMETRY_API void Random(Ellipsoid &ellipsoid, RandomEngine &rng) noexcept;

    ENGINE_GEOMETRY_API void Random(Ellipsoid &ellipsoid) noexcept;
} // namespace engine::geometry
