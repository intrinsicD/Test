#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct Plane;

struct ENGINE_GEOMETRY_API Segment {
    math::vec3 start;
    math::vec3 end;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 direction(const Segment& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float length(const Segment& s) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 point_at(const Segment& s, float t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool intersects(const Segment& s, const Plane& p, float& out_t) noexcept;

}  // namespace engine::geometry

