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

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Normal(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 UnitNormal(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Area(const Triangle &t) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Centroid(const Triangle &t) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle &t, const math::vec3 &point) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle &t, const Segment &segment) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &t, const Aabb &aabb) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &t, const Cylinder &cylinder) noexcept;
    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Triangle &t, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ToBarycentricCoords(const Triangle &t, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 FromBarycentricCoords(const Triangle& t, const math::vec3& bc) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double
    SquaredDistance(const Triangle& triangle, const math::vec3& point) noexcept;
} // namespace engine::geometry
