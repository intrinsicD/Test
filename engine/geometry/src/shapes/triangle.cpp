#include "engine/geometry/shapes/triangle.hpp"
#include "engine/geometry/shapes/segment.hpp"

#include <cmath>

namespace engine::geometry
{
    namespace
    {
        [[nodiscard]] constexpr float half() noexcept { return 0.5f; }
    } // namespace

    math::vec3 Normal(const Triangle& t) noexcept
    {
        return math::cross(t.b - t.a, t.c - t.a);
    }

    math::vec3 UnitNormal(const Triangle& t) noexcept
    {
        return math::normalize(Normal(t));
    }

    double Area(const Triangle& t) noexcept
    {
        return 0.5 * math::length(Normal(t));
    }

    math::vec3 Centroid(const Triangle& t) noexcept
    {
        return (t.a + t.b + t.c) / static_cast<float>(3.0);
    }

    math::vec3 ToBarycentricCoords(const Triangle &t, const math::vec3 &normal, const math::vec3 &point) noexcept
    {
        double ax = math::utils::abs(normal[0]), ay = std::abs(normal[1]), az = std::abs(normal[2]);
        math::vec3 result;
        math::vec3 vu, wu, pu;

        if (ax > ay && ax > az) {
            if (ax != 0) {
                result[1] = (pu[1] * wu[2] - pu[2] * wu[1]) / normal[0];
                result[2] = (vu[1] * pu[2] - vu[2] * pu[1]) / normal[0];
                result[0] = 1 - result[1] - result[2];
            }
        } else if (ay > az) {
            if (ay != 0) {
                result[1] = (pu[2] * wu[0] - pu[0] * wu[2]) / normal[1];
                result[2] = (vu[2] * pu[0] - vu[0] * pu[2]) / normal[1];
                result[0] = 1 - result[1] - result[2];
            }
        } else {
            if (az != 0) {
                result[1] = (pu[0] * wu[1] - pu[1] * wu[0]) / normal[2];
                result[2] = (vu[0] * pu[1] - vu[1] * pu[0]) / normal[2];
                result[0] = 1 - result[1] - result[2];
            }
        }

        return result;
    }

    math::vec3 FromBarycentricCoords(const Triangle &t, const math::vec3 &bc) noexcept
    {
        return bc[0] * t.a + bc[1] * t.b + bc[2] * t.c;
    }

    double SquaredDistance(const Triangle& triangle, const math::vec3& point) noexcept
    {
        const math::vec3 a = triangle.a;
        const math::vec3 b = triangle.b;
        const math::vec3 c = triangle.c;

        const math::vec3 ab = b - a;
        const math::vec3 ac = c - a;
        const math::vec3 ap = point - a;

        const float d1 = math::dot(ab, ap);
        const float d2 = math::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
        {
            return math::length_squared(ap);
        }

        const math::vec3 bp = point - b;
        const float d3 = math::dot(ab, bp);
        const float d4 = math::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
        {
            return math::length_squared(bp);
        }

        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            const float v = d1 / (d1 - d3);
            const math::vec3 projection = a + ab * v;
            return math::length_squared(point - projection);
        }

        const math::vec3 cp = point - c;
        const float d5 = math::dot(ab, cp);
        const float d6 = math::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
        {
            return math::length_squared(cp);
        }

        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            const float w = d2 / (d2 - d6);
            const math::vec3 projection = a + ac * w;
            return math::length_squared(point - projection);
        }

        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            const math::vec3 projection = b + (c - b) * w;
            return math::length_squared(point - projection);
        }

        const math::vec3 normal = math::cross(ab, ac);
        const float normal_len_sq = math::length_squared(normal);
        if (normal_len_sq == 0.0f)
        {
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
