#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct aabb;
struct sphere;

struct ENGINE_GEOMETRY_API obb {
    math::vec3 center;
    math::vec3 half_sizes;
    math::mat3 orientation;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 size(const obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 extent(const obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const obb& box, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const obb& outer, const obb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const obb& outer, const aabb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const obb& outer, const sphere& inner) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const obb& lhs, const obb& rhs) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const obb& box, const aabb& other) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const obb& box, const sphere& s) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API aabb bounding_aabb(const obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API obb from_aabb(const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API obb from_center_half_sizes(const math::vec3& center, const math::vec3& half_sizes) noexcept;

}  // namespace engine::geometry

