#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Aabb;
struct Sphere;

struct ENGINE_GEOMETRY_API Obb {
    math::vec3 center;
    math::vec3 half_sizes;
    math::mat3 orientation;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 size(const Obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 extent(const Obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Obb& box, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Obb& outer, const Obb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Obb& outer, const Aabb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Obb& outer, const Sphere& inner) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Obb& lhs, const Obb& rhs) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Obb& box, const Aabb& other) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Obb& box, const Sphere& s) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API Aabb bounding_aabb(const Obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API Obb from_aabb(const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API Obb from_center_half_sizes(const math::vec3& center, const math::vec3& half_sizes) noexcept;

}  // namespace engine::geometry

