#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Plane;

struct ENGINE_GEOMETRY_API Line {
    math::vec3 point;
    math::vec3 direction;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 point_at(const Line& l, float t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 project_point(const Line& l, const math::vec3& point) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& l, const Plane& p, float& out_t) noexcept;

}  // namespace engine::geometry

