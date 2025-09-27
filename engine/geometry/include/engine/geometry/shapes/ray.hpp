#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct aabb;
struct sphere;
struct plane;

struct ENGINE_GEOMETRY_API ray {
    math::vec3 origin;
    math::vec3 direction;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 point_at(const ray& r, float t) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const ray& r, const aabb& box, float& out_t_min, float& out_t_max) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const ray& r, const sphere& s, float& out_t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const ray& r, const plane& p, float& out_t) noexcept;

}  // namespace engine::geometry

