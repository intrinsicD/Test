#include "engine/geometry/shapes/triangle.hpp"
#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

namespace engine::geometry {
    math::vec3 Normal(const Triangle &triangle) noexcept {
        return math::cross(triangle.b - triangle.a, triangle.c - triangle.a);
    }

    math::vec3 UnitNormal(const Triangle &triangle) noexcept {
        return math::normalize(Normal(triangle));
    }

    double Area(const Triangle &triangle) noexcept {
        return 0.5 * math::length(Normal(triangle));
    }

    math::vec3 Centroid(const Triangle &triangle) noexcept {
        return (triangle.a + triangle.b + triangle.c) / static_cast<float>(3.0);
    }

     bool Contains(const Triangle &t, const math::vec3 &point) noexcept {
        //TODO
    }

    bool Contains(const Triangle &triangle, const Segment &segment) noexcept {
        return Contains(triangle, segment.start) && Contains(triangle, segment.end);
    }

    //------------------------------------------------------------------------------------------------------------------

     bool Intersects(const Triangle &triangle, const Aabb &box) noexcept {
        //TODO
    }

     bool Intersects(const Triangle &triangle, const Obb &box) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Sphere &sphere) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Cylinder &cylinder) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Ellipsoid &ellipsoid) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Line &line) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Plane &plane) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Ray &ray) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Segment &segment) noexcept {
         //TODO
     }

    bool Intersects(const Triangle &triangle, const Triangle &other) noexcept {
         //TODO
     }

    math::vec3 ToBarycentricCoords(const Triangle &triangle, const math::vec3 &normal, const math::vec3 &point) noexcept {
        //TODO: Implement a correct and robust version
    }

    math::vec3 FromBarycentricCoords(const Triangle &triangle, const math::vec3 &bc) noexcept {
        return bc[0] * triangle.a + bc[1] * triangle.b + bc[2] * triangle.c;
    }

    double SquaredDistance(const Triangle &triangle, const math::vec3 &point) noexcept {
        const math::vec3 a = triangle.a;
        const math::vec3 b = triangle.b;
        const math::vec3 c = triangle.c;

        const math::vec3 ab = b - a;
        const math::vec3 ac = c - a;
        const math::vec3 ap = point - a;

        const float d1 = math::dot(ab, ap);
        const float d2 = math::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f) {
            return math::length_squared(ap);
        }

        const math::vec3 bp = point - b;
        const float d3 = math::dot(ab, bp);
        const float d4 = math::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3) {
            return math::length_squared(bp);
        }

        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            const float v = d1 / (d1 - d3);
            const math::vec3 projection = a + ab * v;
            return math::length_squared(point - projection);
        }

        const math::vec3 cp = point - c;
        const float d5 = math::dot(ab, cp);
        const float d6 = math::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) {
            return math::length_squared(cp);
        }

        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            const float w = d2 / (d2 - d6);
            const math::vec3 projection = a + ac * w;
            return math::length_squared(point - projection);
        }

        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            const math::vec3 projection = b + (c - b) * w;
            return math::length_squared(point - projection);
        }

        const math::vec3 normal = math::cross(ab, ac);
        const float normal_len_sq = math::length_squared(normal);
        if (normal_len_sq == 0.0f) {
            // Degenerate triangle, fall back to the minimum of the edges
            const double edge0 = SquaredDistance(Segment{a, b}, point);
            const double edge1 = SquaredDistance(Segment{a, c}, point);
            const double edge2 = SquaredDistance(Segment{b, c}, point);
            return math::utils::min(edge0, edge1, edge2);
        }

        const float distance = math::dot(point - a, normal);
        return static_cast<double>(distance * distance) / static_cast<double>(normal_len_sq);
    }
} // namespace engine::geometry
