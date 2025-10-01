#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Aabb;
struct Sphere;
struct plane;

struct ENGINE_GEOMETRY_API Ray {
    math::vec3 origin;
    math::vec3 direction;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 point_at(const Ray& r, float t) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& r, const Aabb& box, float& out_t_min, float& out_t_max) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& r, const Sphere& s, float& out_t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& r, const plane& p, float& out_t) noexcept;

}  // namespace engine::geometry

