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
    struct Ray;
    struct Segment;
    struct Sphere;
    struct Triangle;

    //all pairwise intersection tests for each shape

    template<typename Shape1, typename Shape2>
    struct ENGINE_GEOMETRY_API Result {
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Aabb, Line> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Aabb, Ray> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Aabb, Segment> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Cylinder, Line> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Cylinder, Ray> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Cylinder, Segment> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Ellipsoid, Line> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Ellipsoid, Ray> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Ellipsoid, Segment> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Line, Line> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Line, Ray> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Line, Segment> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Obb, Line> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Obb, Ray> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Obb, Segment> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Plane, Line> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Plane, Ray> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Plane, Segment> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Ray, Line> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Ray, Ray> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Ray, Segment> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Segment, Line> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Segment, Ray> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Segment, Segment> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Sphere, Line> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Sphere, Ray> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Sphere, Segment> {
        float t_min, t_max;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Triangle, Line> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Triangle, Ray> {
        float t;
    };

    template<>
    struct ENGINE_GEOMETRY_API Result<Triangle, Segment> {
        float t;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Line &b,
                                                      Result<Aabb, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Ray &b,
                                                      Result<Aabb, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Segment &b,
                                                      Result<Aabb, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb &a, const Triangle &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Line &b,
                                                      Result<Cylinder, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Ray &b,
                                                      Result<Cylinder, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Segment &b,
                                                      Result<Cylinder, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder &a, const Triangle &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Line &b,
                                                      Result<Ellipsoid, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Ray &b,
                                                      Result<Ellipsoid, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Segment &b,
                                                      Result<Ellipsoid, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid &a, const Triangle &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Aabb &b,
                                                      Result<Aabb, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Cylinder &b,
                                                      Result<Cylinder, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Ellipsoid &b,
                                                      Result<Ellipsoid, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Line &b,
                                                      Result<Line, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Obb &b,
                                                      Result<Obb, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Plane &b,
                                                      Result<Plane, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Ray &b,
                                                      Result<Line, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Segment &b,
                                                      Result<Line, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Sphere &b,
                                                      Result<Sphere, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line &a, const Triangle &b,
                                                      Result<Triangle, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Line &b,
                                                      Result<Obb, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Ray &b,
                                                      Result<Obb, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Segment &b,
                                                      Result<Obb, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb &a, const Triangle &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Line &b,
                                                      Result<Plane, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Ray &b,
                                                      Result<Plane, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Segment &b,
                                                      Result<Plane, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane &a, const Triangle &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Aabb &b,
                                                      Result<Ray, Aabb> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Cylinder &b,
                                                      Result<Ray, Cylinder> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Ellipsoid &b,
                                                      Result<Ray, Ellipsoid> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Line &b,
                                                      Result<Ray, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Obb &b,
                                                      Result<Ray, Obb> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Plane &b,
                                                      Result<Plane, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Ray &b,
                                                      Result<Ray, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Segment &b,
                                                      Result<Ray, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Sphere &b,
                                                      Result<Ray, Sphere> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray &a, const Triangle &b,
                                                      Result<Ray, Triangle> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Aabb &b,
                                                      Result<Segment, Aabb> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Cylinder &b,
                                                      Result<Segment, Cylinder> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Ellipsoid &b,
                                                      Result<Ellipsoid, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Line &b,
                                                      Result<Segment, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Obb &b,
                                                      Result<Obb, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Plane &b,
                                                      Result<Plane, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Ray &b,
                                                      Result<Segment, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Segment &b,
                                                      Result<Segment, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Sphere &b,
                                                      Result<Segment, Sphere> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &a, const Triangle &b,
                                                      Result<Segment, Triangle> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Line &b,
                                                      Result<Sphere, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Ray &b,
                                                      Result<Sphere, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Segment &b,
                                                      Result<Sphere, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere &a, const Triangle &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Aabb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Cylinder &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Ellipsoid &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Line &b,
                                                      Result<Triangle, Line> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Obb &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Plane &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Ray &b,
                                                      Result<Triangle, Ray> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Segment &b,
                                                      Result<Triangle, Segment> *result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Sphere &b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle &a, const Triangle &b) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const math::vec3 &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Aabb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Cylinder &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Ellipsoid &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Obb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Segment &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Sphere &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb &outer, const Triangle &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const math::vec3 &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Aabb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Cylinder &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Ellipsoid &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Obb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Segment &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Sphere &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder &outer, const Triangle &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const math::vec3 &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Aabb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Cylinder &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Ellipsoid &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Obb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Segment &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Sphere &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid &outer, const Triangle &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const math::vec3 &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Aabb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Cylinder &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Ellipsoid &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Obb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Segment &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Sphere &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb &outer, const Triangle &inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const math::vec3 &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Aabb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Cylinder &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Ellipsoid &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Obb &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Segment &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Sphere &inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere &outer, const Triangle &inner) noexcept;

}
