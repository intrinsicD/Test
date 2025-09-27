#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct plane;

struct ENGINE_GEOMETRY_API segment {
    math::vec3 start;
    math::vec3 end;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 direction(const segment& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float length(const segment& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 point_at(const segment& s, float t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const segment& s, const plane& p, float& out_t) noexcept;

}  // namespace engine::geometry

