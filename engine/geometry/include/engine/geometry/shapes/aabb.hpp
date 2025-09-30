#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Sphere;
struct Obb;

struct ENGINE_GEOMETRY_API Aabb {
    math::vec3 min;
    math::vec3 max;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 center(const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 size(const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 extent(const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float volume(const Aabb& box) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Aabb& box, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Aabb& outer, const Aabb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Aabb& outer, const Sphere& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Aabb& outer, const Obb& inner) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Aabb& lhs, const Aabb& rhs) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Aabb& box, const Sphere& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Aabb& box, const Obb& other) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API Aabb make_aabb_from_point(const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API Aabb make_aabb_from_center_extent(const math::vec3& center, const math::vec3& extent) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API Aabb bounding_aabb(const Sphere& s) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API double squared_distance(const Aabb& box, const math::vec3 &point) noexcept;

}  // namespace engine::geometry

