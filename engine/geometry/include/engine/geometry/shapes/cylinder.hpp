#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct Sphere;

    struct ENGINE_GEOMETRY_API Cylinder {
        math::vec3 center;
        math::vec3 axis;
        float radius;
        float half_height;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 AxisDirection(const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 TopCenter(const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 BottomCenter(const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Volume(const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float LateralSurfaceArea(const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float SurfaceArea(const Cylinder &cylinder) noexcept;
} // namespace engine::geometry
