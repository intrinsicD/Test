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
    struct Sphere;
    struct Triangle;

    struct ENGINE_GEOMETRY_API Segment {
        math::vec3 start;
        math::vec3 end;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 Direction(const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API float Length(const Segment &segment) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 PointAt(const Segment &segment, float t) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Segment &outer, const math::vec3 &inner,
                                                    float epsilon = 1e-4f) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Contains(const Segment &outer, const Segment &inner,
                                                    float epsilon = 1e-4f) noexcept;

    //------------------------------------------------------------------------------------------------------------------

    struct ENGINE_GEOMETRY_API Segment1DHit {
        float t_in, t_out;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Aabb &aabb,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Cylinder &cylinder,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Ellipsoid &ellipsoid,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &outer,
                                                      const Line &inner,
                                                      Segment1DHit *result,
                                                      float epsilon = 1e-4f) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Obb &obb,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Plane &p,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Ray &ray,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Segment &other,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Sphere &sphere,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Segment &segment,
                                                      const Triangle &trianle,
                                                      Segment1DHit *result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 ClosestPoint(const Segment &segment,
                                                              const math::vec3 &point,
                                                              double &t_result) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API double SquaredDistance(const Segment &segment,
                                                             const math::vec3 &point) noexcept;
} // namespace engine::geometry
