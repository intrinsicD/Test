#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Aabb;
struct Obb;
struct Cylinder;

struct ENGINE_GEOMETRY_API Sphere {
    math::vec3 center;
    float radius;
};

[[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const Sphere& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float volume(const Sphere& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& s, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Sphere& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Aabb& inner) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Obb& inner) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& lhs, const Sphere& rhs) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& s, const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& s, const Obb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& s, const Cylinder& c) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API Sphere make_sphere_from_point(const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API Sphere bounding_sphere(const Aabb& box) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API Sphere bounding_sphere(const Obb& box) noexcept;

}  // namespace engine::geometry

