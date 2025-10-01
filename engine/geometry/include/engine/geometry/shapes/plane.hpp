#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Ray;
struct Line;
struct Segment;

struct ENGINE_GEOMETRY_API Plane {
    math::vec3 normal;
    float distance;
};

[[nodiscard]] ENGINE_GEOMETRY_API float signed_distance(const Plane& p, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 project_point(const Plane& p, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& p, const math::vec3& point, float epsilon = 1e-4f) noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& p, const Ray& r, float& out_t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& p, const Line& l, float& out_t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& p, const Segment& s, float& out_t) noexcept;

}  // namespace engine::geometry

