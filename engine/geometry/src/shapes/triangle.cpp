#include "engine/geometry/shapes/triangle.hpp"
#include "engine/geometry/shapes/segment.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/geometry/shapes/ellipsoid.hpp"
#include "engine/geometry/shapes/line.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/geometry/shapes/ray.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"

#include "engine/math/matrix.hpp"
#include "engine/math/utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace
{
    [[nodiscard]] constexpr float epsilon() noexcept { return 1e-6f; }

    [[nodiscard]] constexpr float barycentric_tolerance() noexcept { return 1e-4f; }

    [[nodiscard]] constexpr float plane_tolerance() noexcept { return 1e-4f; }

    struct RayTriangleHit
    {
        float t{};
        engine::math::vec3 barycentric{};
    };

    [[nodiscard]] std::optional<RayTriangleHit> intersect_ray_triangle(const engine::geometry::Triangle& triangle,
                                                                       const engine::math::vec3& origin,
                                                                       const engine::math::vec3& direction) noexcept
    {
        using engine::math::vec3;

        if (engine::math::length_squared(direction) <= epsilon())
        {
            return std::nullopt;
        }

        const vec3 edge0 = triangle.b - triangle.a;
        const vec3 edge1 = triangle.c - triangle.a;
        const vec3 pvec = engine::math::cross(direction, edge1);
        const float det = engine::math::dot(edge0, pvec);

        if (std::fabs(det) <= epsilon())
        {
            return std::nullopt;
        }

        const float inv_det = 1.0f / det;
        const vec3 tvec = origin - triangle.a;
        const float u = engine::math::dot(tvec, pvec) * inv_det;
        if (u < -barycentric_tolerance() || u > 1.0f + barycentric_tolerance())
        {
            return std::nullopt;
        }

        const vec3 qvec = engine::math::cross(tvec, edge0);
        const float v = engine::math::dot(direction, qvec) * inv_det;
        if (v < -barycentric_tolerance() || u + v > 1.0f + barycentric_tolerance())
        {
            return std::nullopt;
        }

        const float t = engine::math::dot(edge1, qvec) * inv_det;
        const float w = 1.0f - u - v;

        vec3 bary{w, u, v};

        // Ensure barycentric values stay within tolerance bounds
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (bary[i] < -barycentric_tolerance() || bary[i] > 1.0f + barycentric_tolerance())
            {
                return std::nullopt;
            }
        }

        return RayTriangleHit{t, bary};
    }

    [[nodiscard]] engine::math::vec2 project_to_axis(const engine::math::vec3& point,
                                                     std::size_t dominant_axis) noexcept
    {
        switch (dominant_axis)
        {
        case 0: return {point[1], point[2]};
        case 1: return {point[0], point[2]};
        default: return {point[0], point[1]};
        }
    }

    [[nodiscard]] float orient2d(const engine::math::vec2& a,
                                 const engine::math::vec2& b,
                                 const engine::math::vec2& c) noexcept
    {
        const engine::math::vec2 ab = b - a;
        const engine::math::vec2 ac = c - a;
        return ab[0] * ac[1] - ab[1] * ac[0];
    }

    [[nodiscard]] bool point_in_triangle_2d(const std::array<engine::math::vec2, 3>& triangle,
                                            const engine::math::vec2& point) noexcept
    {
        const float area = orient2d(triangle[0], triangle[1], triangle[2]);
        if (std::fabs(area) <= epsilon())
        {
            return false;
        }

        const float s = orient2d(point, triangle[0], triangle[1]);
        const float t = orient2d(point, triangle[1], triangle[2]);
        const float u = orient2d(point, triangle[2], triangle[0]);

        if (area > 0.0f)
        {
            return s >= -barycentric_tolerance() && t >= -barycentric_tolerance() && u >= -barycentric_tolerance();
        }
        return s <= barycentric_tolerance() && t <= barycentric_tolerance() && u <= barycentric_tolerance();
    }

    [[nodiscard]] bool on_segment_2d(const engine::math::vec2& a,
                                     const engine::math::vec2& b,
                                     const engine::math::vec2& p) noexcept
    {
        const float min_x = std::min(a[0], b[0]) - barycentric_tolerance();
        const float max_x = std::max(a[0], b[0]) + barycentric_tolerance();
        const float min_y = std::min(a[1], b[1]) - barycentric_tolerance();
        const float max_y = std::max(a[1], b[1]) + barycentric_tolerance();
        if (p[0] < min_x || p[0] > max_x || p[1] < min_y || p[1] > max_y)
        {
            return false;
        }
        return std::fabs(orient2d(a, b, p)) <= barycentric_tolerance();
    }

    [[nodiscard]] bool segments_intersect_2d(const engine::math::vec2& p0,
                                             const engine::math::vec2& p1,
                                             const engine::math::vec2& q0,
                                             const engine::math::vec2& q1) noexcept
    {
        const float o1 = orient2d(p0, p1, q0);
        const float o2 = orient2d(p0, p1, q1);
        const float o3 = orient2d(q0, q1, p0);
        const float o4 = orient2d(q0, q1, p1);

        const auto sign = [](float value)
        {
            if (value > barycentric_tolerance())
            {
                return 1;
            }
            if (value < -barycentric_tolerance())
            {
                return -1;
            }
            return 0;
        };

        const int s1 = sign(o1);
        const int s2 = sign(o2);
        const int s3 = sign(o3);
        const int s4 = sign(o4);

        if (s1 != s2 && s3 != s4)
        {
            return true;
        }

        if (s1 == 0 && on_segment_2d(p0, p1, q0))
        {
            return true;
        }
        if (s2 == 0 && on_segment_2d(p0, p1, q1))
        {
            return true;
        }
        if (s3 == 0 && on_segment_2d(q0, q1, p0))
        {
            return true;
        }
        if (s4 == 0 && on_segment_2d(q0, q1, p1))
        {
            return true;
        }

        return false;
    }

    [[nodiscard]] bool coplanar_triangles_intersect(const engine::geometry::Triangle& lhs,
                                                    const engine::geometry::Triangle& rhs,
                                                    const engine::math::vec3& normal) noexcept
    {
        using engine::math::vec2;
        using engine::math::vec3;

        const vec3 abs_normal{std::fabs(normal[0]), std::fabs(normal[1]), std::fabs(normal[2])};
        std::size_t dominant_axis = 2;
        if (abs_normal[0] > abs_normal[1] && abs_normal[0] > abs_normal[2])
        {
            dominant_axis = 0;
        }
        else if (abs_normal[1] > abs_normal[2])
        {
            dominant_axis = 1;
        }

        std::array<vec2, 3> lhs_proj{
            project_to_axis(lhs.a, dominant_axis),
            project_to_axis(lhs.b, dominant_axis),
            project_to_axis(lhs.c, dominant_axis),
        };
        std::array<vec2, 3> rhs_proj{
            project_to_axis(rhs.a, dominant_axis),
            project_to_axis(rhs.b, dominant_axis),
            project_to_axis(rhs.c, dominant_axis),
        };

        for (const auto& vertex : lhs_proj)
        {
            if (point_in_triangle_2d(rhs_proj, vertex))
            {
                return true;
            }
        }
        for (const auto& vertex : rhs_proj)
        {
            if (point_in_triangle_2d(lhs_proj, vertex))
            {
                return true;
            }
        }

        const std::array<std::pair<int, int>, 3> edges{{{0, 1}, {1, 2}, {2, 0}}};
        for (const auto& [i0, i1] : edges)
        {
            for (const auto& [j0, j1] : edges)
            {
                if (segments_intersect_2d(lhs_proj[i0], lhs_proj[i1], rhs_proj[j0], rhs_proj[j1]))
                {
                    return true;
                }
            }
        }

        return false;
    }

    [[nodiscard]] engine::geometry::Segment make_segment(const engine::math::vec3& a,
                                                         const engine::math::vec3& b) noexcept
    {
        return engine::geometry::Segment{a, b};
    }

    [[nodiscard]] engine::geometry::Segment cylinder_axis_segment(const engine::geometry::Cylinder& cylinder) noexcept
    {
        const engine::math::vec3 axis_dir = engine::geometry::AxisDirection(cylinder);
        return {
            cylinder.center - axis_dir * cylinder.half_height,
            cylinder.center + axis_dir * cylinder.half_height
        };
    }

    [[nodiscard]] bool segment_intersects_cylinder(const engine::geometry::Segment& segment,
                                                   const engine::geometry::Cylinder& cylinder) noexcept
    {
        using engine::math::vec3;

        const vec3 axis_dir = engine::geometry::AxisDirection(cylinder);
        const float axis_len_sq = engine::math::length_squared(axis_dir);
        if (axis_len_sq <= epsilon())
        {
            // Degenerate cylinder – treat as a sphere of the given radius.
            const engine::geometry::Sphere proxy{cylinder.center, cylinder.radius};
            return engine::geometry::Intersects(proxy, segment);
        }

        const vec3 m = segment.start - cylinder.center;
        const vec3 d = segment.end - segment.start;

        const float md = engine::math::dot(m, axis_dir);
        const float nd = engine::math::dot(d, axis_dir);
        const vec3 q = m - axis_dir * md;
        const vec3 qd = d - axis_dir * nd;

        const float a = engine::math::dot(qd, qd);
        const float b = 2.0f * engine::math::dot(q, qd);
        const float c = engine::math::dot(q, q) - cylinder.radius * cylinder.radius;

        const auto check_axial = [&](float t)
        {
            if (t < 0.0f || t > 1.0f)
            {
                return false;
            }
            const float axial = md + nd * t;
            return axial >= -cylinder.half_height - barycentric_tolerance() &&
                axial <= cylinder.half_height + barycentric_tolerance();
        };

        if (std::fabs(a) <= epsilon())
        {
            if (c > 0.0f)
            {
                return false;
            }
            const float end0 = md;
            const float end1 = md + nd;
            const float min_axial = std::min(end0, end1);
            const float max_axial = std::max(end0, end1);
            return !(max_axial < -cylinder.half_height - barycentric_tolerance() ||
                min_axial > cylinder.half_height + barycentric_tolerance());
        }

        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f)
        {
            return false;
        }

        const float sqrt_disc = engine::math::utils::sqrt(discriminant);
        const float inv = 0.5f / a;
        const float t0 = (-b - sqrt_disc) * inv;
        const float t1 = (-b + sqrt_disc) * inv;

        if (check_axial(t0) || check_axial(t1))
        {
            return true;
        }

        const auto check_cap = [&](float cap_height)
        {
            if (std::fabs(nd) <= epsilon())
            {
                return false;
            }
            const float t = (cap_height - md) / nd;
            if (t < 0.0f || t > 1.0f)
            {
                return false;
            }
            const vec3 point = segment.start + d * t;
            const vec3 relative = point - (cylinder.center + axis_dir * cap_height);
            const vec3 radial = relative - axis_dir * engine::math::dot(relative, axis_dir);
            return engine::math::length_squared(radial) <= cylinder.radius * cylinder.radius + barycentric_tolerance();
        };

        return check_cap(cylinder.half_height) || check_cap(-cylinder.half_height);
    }

    [[nodiscard]] bool cylinder_intersects_triangle(const engine::geometry::Cylinder& cylinder,
                                                    const engine::geometry::Triangle& triangle) noexcept
    {
        const engine::math::vec3 axis_dir = engine::geometry::AxisDirection(cylinder);
        const float axis_len_sq = engine::math::length_squared(axis_dir);
        if (axis_len_sq <= epsilon())
        {
            const engine::geometry::Sphere proxy{cylinder.center, cylinder.radius};
            return engine::geometry::Intersects(proxy, triangle);
        }

        const std::array<engine::math::vec3, 3> vertices{triangle.a, triangle.b, triangle.c};
        for (const auto& vertex : vertices)
        {
            if (engine::geometry::Contains(cylinder, vertex))
            {
                return true;
            }
        }

        const std::array<engine::geometry::Segment, 3> edges{
            make_segment(triangle.a, triangle.b),
            make_segment(triangle.b, triangle.c),
            make_segment(triangle.c, triangle.a),
        };
        for (const auto& edge : edges)
        {
            if (segment_intersects_cylinder(edge, cylinder))
            {
                return true;
            }
        }

        const engine::geometry::Segment axis_segment = cylinder_axis_segment(cylinder);
        return engine::geometry::Intersects(triangle, axis_segment);
    }
} // namespace

namespace engine::geometry
{
    math::vec3 Normal(const Triangle& triangle) noexcept
    {
        return math::cross(triangle.b - triangle.a, triangle.c - triangle.a);
    }

    math::vec3 UnitNormal(const Triangle& triangle) noexcept
    {
        return math::normalize(Normal(triangle));
    }

    double Area(const Triangle& triangle) noexcept
    {
        return 0.5 * math::length(Normal(triangle));
    }

    math::vec3 Centroid(const Triangle& triangle) noexcept
    {
        return (triangle.a + triangle.b + triangle.c) / static_cast<float>(3.0);
    }

    math::vec3 ClosestPoint(const Triangle& triangle, const math::vec3& point) noexcept
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
            return a;
        }

        const math::vec3 bp = point - b;
        const float d3 = math::dot(ab, bp);
        const float d4 = math::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
        {
            return b;
        }

        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            const float v = d1 / (d1 - d3);
            return a + ab * v;
        }

        const math::vec3 cp = point - c;
        const float d5 = math::dot(ab, cp);
        const float d6 = math::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
        {
            return c;
        }

        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            const float w = d2 / (d2 - d6);
            return a + ac * w;
        }

        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            const math::vec3 bc = c - b;
            const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + bc * w;
        }

        const math::vec3 n = math::cross(ab, ac);
        const float n_len_sq = math::length_squared(n);
        if (n_len_sq <= epsilon())
        {
            // Degenerate triangle: fall back to closest point among edges
            auto closest_on_segment = [](const math::vec3& p,
                                         const math::vec3& s0,
                                         const math::vec3& s1) -> math::vec3
            {
                const math::vec3 d = s1 - s0;
                const float denom = math::length_squared(d);
                if (denom <= epsilon())
                {
                    return s0;
                }
                float t = math::dot(p - s0, d) / denom;
                t = std::clamp(t, 0.0f, 1.0f);
                return s0 + d * t;
            };

            const math::vec3 p_ab = closest_on_segment(point, a, b);
            const math::vec3 p_ac = closest_on_segment(point, a, c);
            const math::vec3 p_bc = closest_on_segment(point, b, c);

            const float d_ab = math::length_squared(point - p_ab);
            const float d_ac = math::length_squared(point - p_ac);
            const float d_bc = math::length_squared(point - p_bc);

            if (d_ab <= d_ac && d_ab <= d_bc) return p_ab;
            if (d_ac <= d_bc) return p_ac;
            return p_bc;
        }

        // Inside face region: orthogonal projection onto plane
        const float dist = math::dot(ap, n) / n_len_sq;
        return point - n * dist;
    }

    math::vec3 ToBarycentricCoords(const Triangle& triangle, const math::vec3& normal, const math::vec3& point) noexcept
    {
        (void)normal;
        const math::vec3 a = triangle.a;
        const math::vec3 b = triangle.b;
        const math::vec3 c = triangle.c;

        const math::vec3 v0 = b - a;
        const math::vec3 v1 = c - a;
        const math::vec3 v2 = point - a;

        const double d00 = static_cast<double>(math::dot(v0, v0));
        const double d01 = static_cast<double>(math::dot(v0, v1));
        const double d11 = static_cast<double>(math::dot(v1, v1));
        const double d20 = static_cast<double>(math::dot(v2, v0));
        const double d21 = static_cast<double>(math::dot(v2, v1));

        const double denom = d00 * d11 - d01 * d01;
        if (std::fabs(denom) <= static_cast<double>(epsilon()))
        {
            return math::vec3{-std::numeric_limits<float>::infinity()};
        }

        const double inv_denom = 1.0 / denom;
        const double v = (d11 * d20 - d01 * d21) * inv_denom;
        const double w = (d00 * d21 - d01 * d20) * inv_denom;
        const double u = 1.0 - v - w;

        return math::vec3{
            static_cast<float>(u),
            static_cast<float>(v),
            static_cast<float>(w),
        };
    }

    math::vec3 FromBarycentricCoords(const Triangle& triangle, const math::vec3& bc) noexcept
    {
        return bc[0] * triangle.a + bc[1] * triangle.b + bc[2] * triangle.c;
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
