#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct ray;
struct line;
struct segment;

struct ENGINE_GEOMETRY_API plane {
    math::vec3 normal;
    float distance;
};

[[nodiscard]] ENGINE_GEOMETRY_API float signed_distance(const plane& p, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 project_point(const plane& p, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const plane& p, const math::vec3& point, float epsilon = 1e-4f) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const plane& p, const ray& r, float& out_t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const plane& p, const line& l, float& out_t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const plane& p, const segment& s, float& out_t) noexcept;

}  // namespace engine::geometry

