#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

#include <array>

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

    struct ENGINE_GEOMETRY_API Aabb {
        math::vec3 min;
        math::vec3 max;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Center(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Size(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Extent(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SurfaceArea(const Aabb &box) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double Volume(const Aabb &box) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &box, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Aabb &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Sphere &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Obb &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Cylinder &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Ellipsoid &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Segment &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Triangle &inner) noexcept;


    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &lhs, const Aabb &rhs) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Sphere &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Obb &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Cylinder &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Ellipsoid &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Line &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Plane &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Ray &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Segment &other) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &box, const Triangle &other) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb MakeAabbFromCenterExtent(const math::vec3 &center,
                                                                    const math::vec3 &extent) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const Sphere &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const Obb &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const Cylinder &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const Ellipsoid &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const Segment &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const Triangle &s) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb Merge(const Aabb &l, const Aabb &r) noexcept;

    ENGINE_GEOMETRY_API void Merge(Aabb &box, const Aabb &other) noexcept;

    ENGINE_GEOMETRY_API void Merge(Aabb &box, const math::vec3 &point) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Aabb &box, const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Aabb &box, const math::vec3 &point) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] std::array<math::vec3, 8> GetCorners(const Aabb &box) noexcept;

    [[nodiscard]] std::array<math::ivec2, 12> GetEdges(const Aabb &box) noexcept;

    [[nodiscard]] std::array<math::ivec3, 12> GetFaceTriangles(const Aabb &box) noexcept;

    [[nodiscard]] std::array<math::ivec4, 6> GetFaceQuads(const Aabb &box) noexcept;
} // namespace engine::geometry
