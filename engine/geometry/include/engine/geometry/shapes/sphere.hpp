#pragma once

#include <vector>

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"
#include "engine/geometry/octree/octree.hpp"

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

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &sphere, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Sphere &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Aabb &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Obb &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Cylinder &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Ellipsoid &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Segment &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Triangle &inner) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &lhs, const Sphere &rhs) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Line &line) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Ray &ray) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Plane &plane) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Ellipsoid &ellipsoid) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &sphere, const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &spherephere, const Triangle &triangle) noexcept;

    //------------------------------------------------------------------------------------------------------------------

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
} // namespace engine::geometry
