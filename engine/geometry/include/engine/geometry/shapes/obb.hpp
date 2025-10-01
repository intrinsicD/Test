#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/quaternion.hpp"
#include "engine/math/vector.hpp"

#include <array>

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

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &box, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Obb &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Aabb &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Sphere &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &lhs, const Obb &rhs) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &box, const Aabb &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &box, const Sphere &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb FromAabb(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Obb FromCenterHalfSizes(const math::vec3 &center,
                                                              const math::vec3 &half_sizes) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API std::array<math::vec3, 8> GetCorners(const Obb &box) noexcept;
} // namespace engine::geometry
