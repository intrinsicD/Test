#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct aabb;
struct obb;
struct cylinder;

struct ENGINE_GEOMETRY_API sphere {
    math::vec3 center;
    float radius;
};

[[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const sphere& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float volume(const sphere& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const sphere& s, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const sphere& outer, const sphere& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const sphere& outer, const aabb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const sphere& outer, const obb& inner) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const sphere& lhs, const sphere& rhs) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const sphere& s, const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const sphere& s, const obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const sphere& s, const cylinder& c) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API sphere make_sphere_from_point(const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API sphere bounding_sphere(const aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API sphere bounding_sphere(const obb& box) noexcept;

}  // namespace engine::geometry

