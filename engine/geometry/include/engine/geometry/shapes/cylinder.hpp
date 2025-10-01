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

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 AxisDirection(const Cylinder &c) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 TopCenter(const Cylinder &c) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 BottomCenter(const Cylinder &c) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Volume(const Cylinder &c) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float LateralSurfaceArea(const Cylinder &c) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float SurfaceArea(const Cylinder &c) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &c, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &c, const Sphere &s) noexcept;
} // namespace engine::geometry
