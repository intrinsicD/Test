#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct plane;

struct ENGINE_GEOMETRY_API line {
    math::vec3 point;
    math::vec3 direction;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 point_at(const line& l, float t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 project_point(const line& l, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const line& l, const plane& p, float& out_t) noexcept;

}  // namespace engine::geometry

