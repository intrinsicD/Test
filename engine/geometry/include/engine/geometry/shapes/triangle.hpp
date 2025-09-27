#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct ENGINE_GEOMETRY_API triangle {
    math::vec3 a;
    math::vec3 b;
    math::vec3 c;
};

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 normal(const triangle& t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 unit_normal(const triangle& t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API float area(const triangle& t) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 centroid(const triangle& t) noexcept;

}  // namespace engine::geometry

