#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Ellipsoid {
        math::vec3 center;
        math::vec3 radii;
        math::mat3 orientation;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API float Volume(const Ellipsoid &e) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &e, const math::vec3 &point) noexcept;
} // namespace engine::geometry
