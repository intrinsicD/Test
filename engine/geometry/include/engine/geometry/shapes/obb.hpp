#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/quaternion.hpp"
#include "engine/math/vector.hpp"

#include <span>

namespace engine::geometry {
    struct Aabb;
    struct Sphere;

    struct ENGINE_GEOMETRY_API Obb {
        math::vec3 center;
        math::vec3 half_sizes;
        math::quat orientation;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Size(const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Extent(const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SurfaceArea(const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Volume(const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb MakeObbFromCenterHalfSizes(const math::vec3 &center,
                                                                     const math::vec3 &half_sizes,
                                                                     const math::quat &orientation) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb BoundingObb(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb BoundingObb(const Sphere &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb BoundingObb(const Obb &box, const math::mat4 &transform) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb BoundingObb(std::span<math::vec3> points) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Obb &box, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Obb &box, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API std::array<math::vec3, 8> GetCorners(const Obb &box) noexcept;
} // namespace engine::geometry
