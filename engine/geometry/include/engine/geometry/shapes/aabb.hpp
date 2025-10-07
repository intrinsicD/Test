#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <span>

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

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb MakeAabbFromCenterExtent(const math::vec3 &center,
                                                                    const math::vec3 &extent) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(const math::vec3 &point) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(std::span<math::vec3> points) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API Aabb BoundingAabb(std::span<Aabb> aabbs) noexcept;

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

    ENGINE_GEOMETRY_API void Random(Aabb &box, RandomEngine &rng) noexcept;

    ENGINE_GEOMETRY_API void Random(Aabb &box) noexcept;
} // namespace engine::geometry
