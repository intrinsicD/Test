#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct sphere;

struct ENGINE_GEOMETRY_API cylinder {
    math::vec3 center;
    math::vec3 axis;
    float radius;
    float half_height;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 axis_direction(const cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 top_center(const cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 bottom_center(const cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float volume(const cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float lateral_surface_area(const cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const cylinder& c) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool contains(const cylinder& c, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const cylinder& c, const sphere& s) noexcept;

}  // namespace engine::geometry

