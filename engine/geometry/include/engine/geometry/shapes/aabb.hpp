#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct sphere;
struct obb;

struct ENGINE_GEOMETRY_API aabb {
    math::vec3 min;
    math::vec3 max;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 center(const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 size(const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 extent(const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float volume(const aabb& box) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const aabb& box, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const aabb& outer, const aabb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const aabb& outer, const sphere& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const aabb& outer, const obb& inner) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const aabb& lhs, const aabb& rhs) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const aabb& box, const sphere& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const aabb& box, const obb& other) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API aabb make_aabb_from_point(const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API aabb make_aabb_from_center_extent(const math::vec3& center, const math::vec3& extent) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API aabb bounding_aabb(const sphere& s) noexcept;

}  // namespace engine::geometry

