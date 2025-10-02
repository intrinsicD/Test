#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct Cylinder;
    struct Ellipsoid;
    struct Line;
    struct Obb;
    struct Plane;
    struct Ray;
    struct Segment;
    struct Sphere;
    struct Triangle;
    struct Aabb;

    struct ENGINE_GEOMETRY_API Triangle {
        math::vec3 a;
        math::vec3 b;
        math::vec3 c;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Normal(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 UnitNormal(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Area(const Triangle &triangle) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Centroid(const Triangle &triangle) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle &outer, const math::vec3 &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle &outer, const Triangle &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle &outer, const Segment &inner) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Obb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Sphere &sphere) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Cylinder &cylinder) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Ellipsoid &ellipsoid) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Line &line) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Plane &plane) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Ray &ray) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &triangle, const Triangle &other) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Triangle &triangle,
                                                              const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ToBarycentricCoords(const Triangle &triangle,
                                                                     const math::vec3 &normal,
                                                                     const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 FromBarycentricCoords(const Triangle &triangle,
                                                                       const math::vec3 &bc) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Triangle &triangle,
                                                             const math::vec3 &point) noexcept;
} // namespace engine::geometry
