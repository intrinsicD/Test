#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Ray {
        math::vec3 origin;
        math::vec3 direction;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 PointAt(const Ray &r, float t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Ray &r, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Ray &ray, const math::vec3 &point) noexcept;

    ENGINE_GEOMETRY_API void Random(Ray &ray, RandomEngine &rng) noexcept;

    ENGINE_GEOMETRY_API void Random(Ray &ray) noexcept;
} // namespace engine::geometry
