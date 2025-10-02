#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {
    struct Aabb;
    struct Cylinder;
    struct Ellipsoid;
    struct Line;
    struct Obb;
    struct Plane;
    struct Segment;
    struct Sphere;
    struct Triangle;

    struct ENGINE_GEOMETRY_API Ray {
        math::vec3 origin;
        math::vec3 direction;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 PointAt(const Ray &r, float t) noexcept;

    struct ENGINE_GEOMETRY_API RayHit {
        float t_in, t_out;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Aabb &box,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Cylinder &cylinder,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray, const
                                                      Ellipsoid &ellipsoid,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Line &line,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Obb &obb,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Plane &plane,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Ray &other,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Sphere &sphere,
                                                      RayHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &ray,
                                                      const Triangle &triangle,
                                                      RayHit *result) noexcept;
} // namespace engine::geometry
