#pragma once

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry::constants
{
    constexpr float INTERSECTION_EPSILON = 1e-8f;
    constexpr float SEPARATION_EPSILON = 1e-6f;
    constexpr float PARALLEL_EPSILON = 1e-8f;
}

namespace engine::geometry
{
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
    struct ENGINE_GEOMETRY_API Result
    {
        union
        {
            float t; // for single intersection point
            // for two intersection points
            struct
            {
                float t_min, t_max;
            };
        };
    };

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& a, const Triangle& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Cylinder& a, const Triangle& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ellipsoid& a, const Triangle& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Aabb& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Cylinder& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Ellipsoid& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Obb& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Plane& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Sphere& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Line& a, const Triangle& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& a, const Triangle& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Plane& a, const Triangle& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Aabb& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Cylinder& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Ellipsoid& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Obb& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Plane& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Sphere& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Ray& a, const Triangle& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Aabb& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Cylinder& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Ellipsoid& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Obb& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Plane& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Sphere& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment& a, const Triangle& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& a, const Triangle& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Aabb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Cylinder& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Ellipsoid& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Line& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Obb& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Plane& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Ray& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Segment& b,
                                                      Result* result = nullptr) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Sphere& b) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Triangle& a, const Triangle& b) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const math::vec3& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Aabb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Cylinder& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Ellipsoid& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Obb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Segment& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Sphere& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Aabb& outer, const Triangle& inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const math::vec3& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Aabb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Cylinder& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Ellipsoid& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Obb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Segment& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Sphere& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Cylinder& outer, const Triangle& inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const math::vec3& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Aabb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Cylinder& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Ellipsoid& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Obb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Segment& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Sphere& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Ellipsoid& outer, const Triangle& inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const math::vec3& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Aabb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Cylinder& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Ellipsoid& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Obb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Segment& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Sphere& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Obb& outer, const Triangle& inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& outer, const math::vec3& inner,
                                                    float eps = constants::PARALLEL_EPSILON) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& outer, const Line& inner,
                                                    float eps = constants::PARALLEL_EPSILON) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& outer, const Plane& inner,
                                                    float eps = constants::PARALLEL_EPSILON) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& outer, const Ray& inner,
                                                    float eps = constants::PARALLEL_EPSILON) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& outer, const Segment& inner,
                                                    float eps = constants::PARALLEL_EPSILON) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Plane& outer, const Triangle& inner,
                                                    float eps = constants::PARALLEL_EPSILON) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const math::vec3& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Aabb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Cylinder& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Ellipsoid& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Obb& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Segment& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Sphere& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Sphere& outer, const Triangle& inner) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle& outer, const math::vec3& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle& outer, const Triangle& inner) noexcept;
    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Triangle& outer, const Segment& inner) noexcept;
}
