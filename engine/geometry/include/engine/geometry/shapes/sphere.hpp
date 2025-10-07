#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/vector.hpp"

#include <vector>

namespace engine::geometry {
    struct Aabb;
    struct Cylinder;
    struct Ellipsoid;
    struct Line;
    struct Obb;
    struct Plane;
    struct Ray;
    struct Segment;
    struct Triangle;

    struct ENGINE_GEOMETRY_API Sphere {
        math::vec3 center;
        float radius = 0.0f;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API double SurfaceArea(const Sphere &sphere) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Volume(const Sphere &sphere) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const Ellipsoid &ellipsoid) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere BoundingSphere(const Triangle &triangle) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API Sphere Merge(const Sphere &l, const Sphere &r) noexcept;

    ENGINE_GEOMETRY_API void Merge(Sphere &sphere, const Sphere &other) noexcept;

    ENGINE_GEOMETRY_API void Merge(Sphere &sphere, const math::vec3 &point) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Sphere &sphere, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Sphere &sphere, const math::vec3 &point) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    ENGINE_GEOMETRY_API void Random(Sphere &sphere, RandomEngine &rng) noexcept;

    ENGINE_GEOMETRY_API void Random(Sphere &sphere) noexcept;
} // namespace engine::geometry
