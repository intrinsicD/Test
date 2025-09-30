#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Sphere;

struct ENGINE_GEOMETRY_API Cylinder {
    math::vec3 center;
    math::vec3 axis;
    float radius;
    float half_height;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 axis_direction(const Cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 top_center(const Cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 bottom_center(const Cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float volume(const Cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float lateral_surface_area(const Cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const Cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const Cylinder& c, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Cylinder& c, const Sphere& s) noexcept;

}  // namespace engine::geometry

