#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/shapes.hpp"
#include "engine/math/utils.hpp"
#include "engine/math/utils_rotation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace engine::geometry
{
    bool Intersects(const Aabb& a, const Aabb& b) noexcept
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (a.max[i] < b.min[i] || a.min[i] > b.max[i])
            {
                return false;
            }
        }
        return true;
    }

    bool Intersects(const Aabb& a, const Cylinder& b) noexcept
    {
        // Find closest point on AABB to cylinder axis
        const math::vec3 axis_dir = AxisDirection(b);
        const math::vec3 axis_start = b.center - axis_dir * b.half_height;
        const math::vec3 axis_end = b.center + axis_dir * b.half_height;

        // Clamp axis endpoints to AABB
        math::vec3 p1 = axis_start;
        math::vec3 p2 = axis_end;

        for (std::size_t i = 0; i < 3; ++i)
        {
            p1[i] = math::utils::clamp(p1[i], a.min[i], a.max[i]);
            p2[i] = math::utils::clamp(p2[i], a.min[i], a.max[i]);
        }

        // Find closest point on segment to cylinder center
        const math::vec3 seg_dir = p2 - p1;
        const math::vec3 to_center = b.center - p1;
        const float seg_len_sq = math::dot(seg_dir, seg_dir);

        math::vec3 closest;
        if (math::utils::nearly_equal(seg_len_sq, 0.0f, constants::PARALLEL_EPSILON))
        {
            closest = p1;
        }
        else
        {
            float t = math::dot(to_center, seg_dir) / seg_len_sq;
            t = math::utils::clamp(t, 0.0f, 1.0f);
            closest = p1 + seg_dir * t;
        }

        // Check if closest point is within cylinder
        const math::vec3 delta = closest - b.center;
        const float axial = math::dot(delta, axis_dir);

        if (math::utils::abs(axial) > b.half_height)
            return false;

        const math::vec3 radial_vec = delta - axial * axis_dir;
        const float radial_dist_sq = math::dot(radial_vec, radial_vec);

        return radial_dist_sq <= b.radius * b.radius;
    }

    bool Intersects(const Aabb& a, const Ellipsoid& b) noexcept
    {
        // Conservative test: check if AABB intersects the ellipsoid's bounding sphere
        // TODO: For a more precise test, we'd need to transform the AABB into ellipsoid space

        const math::mat3 R = math::utils::to_rotation_matrix(b.orientation);

        // Find the closest point on AABB to ellipsoid center
        const math::vec3 closest = ClosestPoint(a, b.center);

        // Transform closest point to ellipsoid local space
        const math::mat3 Rt = transpose(R);
        const math::vec3 local = Rt * (closest - b.center);

        // Scale by inverse radii to get unit sphere space
        const math::vec3 scaled{
            local[0] / b.radii[0],
            local[1] / b.radii[1],
            local[2] / b.radii[2]
        };

        // Check if within unit sphere
        return math::dot(scaled, scaled) <= 1.0f;
    }

    bool Intersects(const Aabb& box, const Line& ln, Result* result) noexcept
    {
        const float INF = std::numeric_limits<float>::infinity();
        float tmin = -INF, tmax = +INF;

        for (std::size_t i = 0; i < 3; ++i)
        {
            const float o = ln.point[i];
            const float d = ln.direction[i];
            //TODO: check if this is the correct epsilon to use
            if (math::utils::nearly_equal(d, 0.0f, constants::SEPARATION_EPSILON))
            {
                if (o < box.min[i] || o > box.max[i]) return false;
            }
            else
            {
                float inv = 1.0f / d;
                float t0 = (box.min[i] - o) * inv;
                float t1 = (box.max[i] - o) * inv;
                if (t0 > t1)
                {
                    const float tmp = t0;
                    t0 = t1;
                    t1 = tmp;
                }
                if (t0 > tmin) tmin = t0;
                if (t1 < tmax) tmax = t1;
                if (tmax < tmin) return false;
            }
        }
        if (result)
        {
            result->t_min = tmin;
            result->t_max = tmax;
        }
        return true;
    }

    bool Intersects(const Aabb& aabb, const Obb& obb) noexcept
    {
        const math::mat3 B = math::utils::to_rotation_matrix(obb.orientation); // localB->world
        math::mat3 R = B; // since RA=I
        math::mat3 AbsR{};
        const float EPS = 1e-6f;
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) AbsR[i][j] = math::utils::abs(R[i][j]) + EPS;

        const math::vec3 aC = Center(aabb);
        const math::vec3 aE = Extent(aabb);
        const math::vec3 bE = obb.half_sizes;
        const math::vec3 T = (obb.center - aC); // already in A-space (world)

        // A's axes
        for (int i = 0; i < 3; ++i)
        {
            const float ra = aE[i];
            const float rb = bE[0] * AbsR[i][0] + bE[1] * AbsR[i][1] + bE[2] * AbsR[i][2];
            if (math::utils::abs(T[i]) > ra + rb) return false;
        }
        // B's axes
        for (int j = 0; j < 3; ++j)
        {
            const float ra = aE[0] * AbsR[0][j] + aE[1] * AbsR[1][j] + aE[2] * AbsR[2][j];
            const float rb = bE[j];
            const float t = math::utils::abs(T[0] * R[0][j] + T[1] * R[1][j] + T[2] * R[2][j]);
            if (t > ra + rb) return false;
        }
        // Cross axes A_i x B_j
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
            {
                const int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
                const int j1 = (j + 1) % 3, j2 = (j + 2) % 3;
                const float ra = aE[i1] * AbsR[i2][j] + aE[i2] * AbsR[i1][j];
                const float rb = bE[j1] * AbsR[i][j2] + bE[j2] * AbsR[i][j1];
                const float t = math::utils::abs(T[i2] * R[i1][j] - T[i1] * R[i2][j]);
                if (t > ra + rb) return false;
            }
        return true;
    }

    bool Intersects(const Aabb& aabb, const Plane& plane) noexcept
    {
        const math::vec3 c = Center(aabb);
        const math::vec3 e = Extent(aabb);
        const float s = math::dot(plane.normal, c) + plane.distance;
        const math::vec3 an{
            math::utils::abs(plane.normal[0]), math::utils::abs(plane.normal[1]), math::utils::abs(plane.normal[2])
        };
        const float r = e[0] * an[0] + e[1] * an[1] + e[2] * an[2];
        return math::utils::abs(s) <= r;
    }

    bool Intersects(const Aabb& box, const Ray& ray, Result* result) noexcept
    {
        const float INF = std::numeric_limits<float>::infinity();
        float tmin = 0.0f, tmax = +INF;

        for (std::size_t i = 0; i < 3; ++i)
        {
            const float o = ray.origin[i];
            const float d = ray.direction[i];
            //TODO: check if this is the correct epsilon to use
            if (math::utils::nearly_equal(d, 0.0f, constants::SEPARATION_EPSILON))
            {
                if (o < box.min[i] || o > box.max[i]) return false;
            }
            else
            {
                float inv = 1.0f / d;
                float t0 = (box.min[i] - o) * inv;
                float t1 = (box.max[i] - o) * inv;
                if (t0 > t1)
                {
                    const float tmp = t0;
                    t0 = t1;
                    t1 = tmp;
                }
                if (t0 > tmin) tmin = t0;
                if (t1 < tmax) tmax = t1;
                if (tmax < tmin) return false;
            }
        }
        if (result)
        {
            result->t_min = tmin;
            result->t_max = tmax;
        }
        return true;
    }

    bool Intersects(const Aabb& box, const Segment& seg, Result* result) noexcept
    {
        const math::vec3 dir = Direction(seg); // end-start
        const math::vec3 o = seg.start;

        float tmin = 0.0f, tmax = 1.0f;

        for (std::size_t i = 0; i < 3; ++i)
        {
            const float d = dir[i];
            const float oi = o[i];
            //TODO: check if this is the correct epsilon to use
            if (math::utils::nearly_equal(d, 0.0f, constants::SEPARATION_EPSILON))
            {
                if (oi < box.min[i] || oi > box.max[i]) return false;
            }
            else
            {
                const float inv = 1.0f / d;
                float t0 = (box.min[i] - oi) * inv;
                float t1 = (box.max[i] - oi) * inv;
                if (t0 > t1)
                {
                    const float tmp = t0;
                    t0 = t1;
                    t1 = tmp;
                }
                if (t0 > tmin) tmin = t0;
                if (t1 < tmax) tmax = t1;
                if (tmax < tmin) return false;
            }
        }
        if (result)
        {
            result->t_min = tmin;
            result->t_max = tmax;
        }
        return true;
    }

    bool Intersects(const Aabb& aabb, const Sphere& sphere) noexcept
    {
        return SquaredDistance(aabb, sphere.center) <= sphere.radius * sphere.radius; // Ericson ch.5
    }

    bool Intersects(const Aabb& aabb, const Triangle& triangle) noexcept
    {
        // Move triangle so that AABB center is at origin
        const math::vec3 c = Center(aabb);
        const math::vec3 e = Extent(aabb);
        const math::vec3 v0 = triangle.a - c;
        const math::vec3 v1 = triangle.b - c;
        const math::vec3 v2 = triangle.c - c;

        const math::vec3 f0 = v1 - v0;
        const math::vec3 f1 = v2 - v1;
        const math::vec3 f2 = v0 - v2;
        // (Following the original TAM code layout)
        // Axis L = (1,0,0) x f_i
        {
            float p0 = v0[2] * f0[1] - v0[1] * f0[2];
            float p1 = v1[2] * f0[1] - v1[1] * f0[2];
            float p2 = v2[2] * f0[1] - v2[1] * f0[2];
            float r = e[1] * math::utils::abs(f0[2]) + e[2] * math::utils::abs(f0[1]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[2] * f1[1] - v0[1] * f1[2];
            float p1 = v1[2] * f1[1] - v1[1] * f1[2];
            float p2 = v2[2] * f1[1] - v2[1] * f1[2];
            float r = e[1] * math::utils::abs(f1[2]) + e[2] * math::utils::abs(f1[1]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[2] * f2[1] - v0[1] * f2[2];
            float p1 = v1[2] * f2[1] - v1[1] * f2[2];
            float p2 = v2[2] * f2[1] - v2[1] * f2[2];
            float r = e[1] * math::utils::abs(f2[2]) + e[2] * math::utils::abs(f2[1]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }

        // Axis L = (0,1,0) x f_i
        {
            float p0 = v0[0] * f0[2] - v0[2] * f0[0];
            float p1 = v1[0] * f0[2] - v1[2] * f0[0];
            float p2 = v2[0] * f0[2] - v2[2] * f0[0];
            float r = e[0] * math::utils::abs(f0[2]) + e[2] * math::utils::abs(f0[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[0] * f1[2] - v0[2] * f1[0];
            float p1 = v1[0] * f1[2] - v1[2] * f1[0];
            float p2 = v2[0] * f1[2] - v2[2] * f1[0];
            float r = e[0] * math::utils::abs(f1[2]) + e[2] * math::utils::abs(f1[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[0] * f2[2] - v0[2] * f2[0];
            float p1 = v1[0] * f2[2] - v1[2] * f2[0];
            float p2 = v2[0] * f2[2] - v2[2] * f2[0];
            float r = e[0] * math::utils::abs(f2[2]) + e[2] * math::utils::abs(f2[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }

        // Axis L = (0,0,1) x f_i
        {
            float p0 = v0[1] * f0[0] - v0[0] * f0[1];
            float p1 = v1[1] * f0[0] - v1[0] * f0[1];
            float p2 = v2[1] * f0[0] - v2[0] * f0[1];
            float r = e[0] * math::utils::abs(f0[1]) + e[1] * math::utils::abs(f0[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[1] * f1[0] - v0[0] * f1[1];
            float p1 = v1[1] * f1[0] - v1[0] * f1[1];
            float p2 = v2[1] * f1[0] - v2[0] * f1[1];
            float r = e[0] * math::utils::abs(f1[1]) + e[1] * math::utils::abs(f1[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[1] * f2[0] - v0[0] * f2[1];
            float p1 = v1[1] * f2[0] - v1[0] * f2[1];
            float p2 = v2[1] * f2[0] - v2[0] * f2[1];
            float r = e[0] * math::utils::abs(f2[1]) + e[1] * math::utils::abs(f2[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }

        // Test the three AABB axes
        for (int i = 0; i < 3; ++i)
        {
            float minv = math::utils::min(v0[i], v1[i], v2[i]);
            float maxv = math::utils::max(v0[i], v1[i], v2[i]);
            if (minv > e[i] || maxv < -e[i]) return false;
        }

        // Triangle plane vs AABB
        const math::vec3 n = math::cross(f0, f1);
        const float d = math::dot(n, v0);
        const math::vec3 an{math::utils::abs(n[0]), math::utils::abs(n[1]), math::utils::abs(n[2])};
        const float r = e[0] * an[0] + e[1] * an[1] + e[2] * an[2];
        if (d > r || d < -r) return false;

        return true;
    }

    bool Intersects(const Cylinder& a, const Aabb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Cylinder& a, const Cylinder& b) noexcept
    {
        // Check if bounding spheres intersect first (quick reject)
        const Sphere sphere_a = BoundingSphere(a);
        const Sphere sphere_b = BoundingSphere(b);
        if (!Intersects(sphere_a, sphere_b)) return false;

        // Check if any endpoint is inside the other cylinder
        if (Contains(a, TopCenter(b)) || Contains(a, BottomCenter(b))) return true;
        if (Contains(b, TopCenter(a)) || Contains(b, BottomCenter(a))) return true;

        // Check distance between axis segments
        const Segment segment{BottomCenter(a), TopCenter(a)};
        const Segment other{BottomCenter(b), TopCenter(b)};

        double t;
        const math::vec3 closest = ClosestPoint(other, segment.start, t);
        const float dist_sq = SquaredDistance(segment, closest);
        const float radius_sum = a.radius + b.radius;

        return dist_sq <= radius_sum * radius_sum;
    }

    bool Intersects(const Cylinder& a, const Ellipsoid& b) noexcept
    {
        // Check if cylinder axis intersects ellipsoid
        const Line axis_line{a.center, AxisDirection(a)};
        if (Intersects(axis_line, b, nullptr))
        {
            // Further check if intersection is within cylinder height
            const Segment axis_seg{BottomCenter(a), TopCenter(a)};
            if (Intersects(axis_seg, b, nullptr)) return true;
        }

        // Check closest point on ellipsoid to cylinder axis
        const math::vec3 top = TopCenter(a);
        const math::vec3 bottom = BottomCenter(a);

        for (int i = 0; i <= 8; ++i)
        {
            const float t = i / 8.0f;
            const math::vec3 axis_point = bottom + t * (top - bottom);
            if (SquaredDistance(b, axis_point) <= a.radius * a.radius) return true;
        }

        return false;
    }

    bool Intersects(const Cylinder& cylinder, const Line& line, Result* result) noexcept
    {
        const math::vec3 axis_dir = AxisDirection(cylinder);
        const math::vec3 w = line.point - cylinder.center;
        const math::vec3 d = line.direction;

        const float a_dot_d = math::dot(axis_dir, d);
        const float a_dot_w = math::dot(axis_dir, w);

        const math::vec3 d_perp = d - a_dot_d * axis_dir;
        const math::vec3 w_perp = w - a_dot_w * axis_dir;

        const float a = math::dot(d_perp, d_perp);
        const float b = 2.0f * math::dot(w_perp, d_perp);
        const float c = math::dot(w_perp, w_perp) - cylinder.radius * cylinder.radius;

        if (math::utils::nearly_equal(a, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Line parallel to cylinder axis
            if (c > 0.0f) return false; // Outside radius

            // Check height bounds
            const float t1 = (-cylinder.half_height - a_dot_w) / a_dot_d;
            const float t2 = (cylinder.half_height - a_dot_w) / a_dot_d;

            if (result)
            {
                result->t_min = math::utils::min(t1, t2);
                result->t_max = math::utils::max(t1, t2);
            }
            return true;
        }

        const float disc = b * b - 4 * a * c;
        if (disc < 0.0f) return false;

        const float sqrt_disc = math::utils::sqrt(disc);
        float t0 = (-b - sqrt_disc) / (2 * a);
        float t1 = (-b + sqrt_disc) / (2 * a);

        // Check height constraints
        const float h0 = a_dot_w + t0 * a_dot_d;
        const float h1 = a_dot_w + t1 * a_dot_d;

        if (math::utils::abs(h0) > cylinder.half_height && math::utils::abs(h1) > cylinder.half_height)
        {
            if ((h0 > 0) == (h1 > 0)) return false;
        }

        // Clip to height bounds
        if (math::utils::abs(h0) > cylinder.half_height)
        {
            const float h_target = (h0 > 0) ? cylinder.half_height : -cylinder.half_height;
            t0 = (h_target - a_dot_w) / a_dot_d;
        }
        if (math::utils::abs(h1) > cylinder.half_height)
        {
            const float h_target = (h1 > 0) ? cylinder.half_height : -cylinder.half_height;
            t1 = (h_target - a_dot_w) / a_dot_d;
        }

        if (result)
        {
            result->t_min = math::utils::min(t0, t1);
            result->t_max = math::utils::max(t0, t1);
        }
        return true;
    }

    namespace
    {
        struct SegmentAabbClosestResult
        {
            float distance_sq;
            math::vec3 point_on_segment;
            math::vec3 point_on_box;
        };

        SegmentAabbClosestResult ClosestPointSegmentAabb(const Segment& segment,
                                                          const Aabb& box,
                                                          float allowed_t_min,
                                                          float allowed_t_max) noexcept
        {
            float t_min = math::utils::clamp(allowed_t_min, 0.0f, 1.0f);
            float t_max = math::utils::clamp(allowed_t_max, 0.0f, 1.0f);
            if (t_max < t_min) std::swap(t_min, t_max);

            SegmentAabbClosestResult best{std::numeric_limits<float>::infinity(), {}, {}};

            Result seg_result{};
            if (Intersects(box, segment, &seg_result))
            {
                const float entry = math::utils::max(seg_result.t_min, t_min);
                const float exit = math::utils::min(seg_result.t_max, t_max);
                if (exit >= entry)
                {
                    const float t = math::utils::clamp(entry, t_min, t_max);
                    const math::vec3 point = PointAt(segment, t);
                    return {0.0f, point, point};
                }
            }

            const math::vec3 dir = Direction(segment);

            auto evaluate = [&](float t) noexcept
            {
                if (t < t_min || t > t_max) return;

                const math::vec3 point = PointAt(segment, t);
                const math::vec3 box_point = ClosestPoint(box, point);
                const math::vec3 delta = point - box_point;
                const float dist_sq = math::dot(delta, delta);
                if (dist_sq < best.distance_sq)
                {
                    best = {dist_sq, point, box_point};
                }
            };

            evaluate(t_min);
            evaluate(t_max);

            for (std::size_t i = 0; i < 3; ++i)
            {
                const float d = dir[i];
                if (math::utils::nearly_equal(d, 0.0f, constants::PARALLEL_EPSILON)) continue;

                const float inv_d = 1.0f / d;
                const float t0 = (box.min[i] - segment.start[i]) * inv_d;
                const float t1 = (box.max[i] - segment.start[i]) * inv_d;

                evaluate(t0);
                evaluate(t1);
            }

            if (!std::isfinite(best.distance_sq))
            {
                const float mid = 0.5f * (t_min + t_max);
                const math::vec3 point = PointAt(segment, mid);
                const math::vec3 box_point = ClosestPoint(box, point);
                const math::vec3 delta = point - box_point;
                const float dist_sq = math::dot(delta, delta);
                best = {dist_sq, point, box_point};
            }

            return best;
        }
    } // namespace

    bool Intersects(const Cylinder& cyl, const Obb& obb) noexcept
    {
        // Check if any OBB vertex is inside cylinder
        const math::vec3 axis_dir = AxisDirection(cyl);
        const auto corners = GetCorners(obb);
        float obb_proj_min = std::numeric_limits<float>::infinity();
        float obb_proj_max = -std::numeric_limits<float>::infinity();
        for (const auto& corner : corners)
        {
            if (Contains(cyl, corner)) return true;
            const float proj = math::dot(corner - cyl.center, axis_dir);
            obb_proj_min = math::utils::min(obb_proj_min, proj);
            obb_proj_max = math::utils::max(obb_proj_max, proj);
        }

        if (obb_proj_min > cyl.half_height || obb_proj_max < -cyl.half_height) return false;

        // Check if cylinder endpoints are inside OBB
        if (Contains(obb, TopCenter(cyl)) || Contains(obb, BottomCenter(cyl))) return true;

        // Check distance between cylinder axis and OBB
        const Segment axis_seg{BottomCenter(cyl), TopCenter(cyl)};

        const math::mat3 rotation = math::utils::to_rotation_matrix(obb.orientation);
        const math::mat3 rotation_t = transpose(rotation);
        const Segment local_axis{
            rotation_t * (axis_seg.start - obb.center),
            rotation_t * (axis_seg.end - obb.center)
        };
        const Aabb local_box{
            math::vec3{-obb.half_sizes[0], -obb.half_sizes[1], -obb.half_sizes[2]},
            obb.half_sizes
        };
        float segment_t_min = 0.0f;
        float segment_t_max = 1.0f;
        if (cyl.half_height > constants::PARALLEL_EPSILON)
        {
            const float overlap_start = math::utils::max(-cyl.half_height, obb_proj_min);
            const float overlap_end = math::utils::min(cyl.half_height, obb_proj_max);
            const float inv_height = 1.0f / (2.0f * cyl.half_height);
            segment_t_min = (overlap_start + cyl.half_height) * inv_height;
            segment_t_max = (overlap_end + cyl.half_height) * inv_height;
        }

        const auto closest = ClosestPointSegmentAabb(local_axis, local_box, segment_t_min, segment_t_max);
        if (closest.distance_sq <= 0.0f) return true;

        const math::vec3 axis_point_world = rotation * closest.point_on_segment + obb.center;
        const math::vec3 box_point_world = rotation * closest.point_on_box + obb.center;
        const math::vec3 delta = axis_point_world - box_point_world;
        const float axial_component = math::dot(delta, axis_dir);
        const math::vec3 radial_vec = delta - axial_component * axis_dir;
        const float radial_dist_sq = math::dot(radial_vec, radial_vec);

        return radial_dist_sq <= cyl.radius * cyl.radius;
    }

    bool Intersects(const Cylinder& cyl, const Plane& plane) noexcept
    {
        const math::vec3 top = TopCenter(cyl);
        const math::vec3 bottom = BottomCenter(cyl);

        const float d_top = SignedDistance(plane, top);
        const float d_bottom = SignedDistance(plane, bottom);

        // Check if endpoints are on opposite sides
        if (d_top * d_bottom <= 0.0f) return true;

        // Check if cylinder edge touches plane
        const float min_dist = math::utils::min(math::utils::abs(d_top), math::utils::abs(d_bottom));
        return min_dist <= cyl.radius;
    }

    bool Intersects(const Cylinder& cylinder, const Ray& ray, Result* result) noexcept
    {
        Result line_result{};
        const Line line{ray.origin, ray.direction};

        if (!Intersects(cylinder, line, &line_result)) return false;

        // Clamp to ray (t >= 0)
        if (line_result.t_max < 0.0f) return false;

        if (result)
        {
            result->t_min = math::utils::max(0.0f, line_result.t_min);
            result->t_max = line_result.t_max;
        }
        return true;
    }

    bool Intersects(const Cylinder& cyl, const Segment& seg, Result* result) noexcept
    {
        Result line_result{};
        const Line line{seg.start, Direction(seg)};

        if (!Intersects(cyl, line, &line_result)) return false;

        // Clamp to segment (0 <= t <= 1)
        if (line_result.t_min > 1.0f || line_result.t_max < 0.0f) return false;

        if (result)
        {
            result->t_min = math::utils::max(0.0f, line_result.t_min);
            result->t_max = math::utils::min(1.0f, line_result.t_max);
        }
        return true;
    }

    bool Intersects(const Cylinder& cylinder, const Sphere& sphere) noexcept
    {
        const math::vec3 axis_dir = AxisDirection(cylinder);
        const math::vec3 delta = sphere.center - cylinder.center;

        const float axial = math::dot(delta, axis_dir);
        const float abs_axial = math::utils::abs(axial);
        const float axial_excess = abs_axial <= cylinder.half_height
                                       ? 0.0f
                                       : abs_axial - cylinder.half_height;

        const math::vec3 radial_vec = delta - axial * axis_dir;
        const float radial_len_sq = math::dot(radial_vec, radial_vec);
        float radial_excess = 0.0f;
        if (radial_len_sq > cylinder.radius * cylinder.radius)
        {
            const float radial_len = math::utils::sqrt(radial_len_sq);
            radial_excess = radial_len - cylinder.radius;
        }

        const float separation_sq = radial_excess * radial_excess + axial_excess * axial_excess;
        return separation_sq <= sphere.radius * sphere.radius;
    }

    bool Intersects(const Cylinder& cyl, const Triangle& tri) noexcept
    {
        // Check if any triangle vertex is inside cylinder
        if (Contains(cyl, tri.a) || Contains(cyl, tri.b) || Contains(cyl, tri.c)) return true;

        // Check if cylinder axis intersects triangle
        const Segment axis_seg{BottomCenter(cyl), TopCenter(cyl)};
        if (Intersects(axis_seg, tri, nullptr)) return true;

        // Check if triangle edges intersect cylinder
        const Segment edges[3] = {
            {tri.a, tri.b},
            {tri.b, tri.c},
            {tri.c, tri.a}
        };

        for (const auto& edge : edges)
        {
            if (Intersects(cyl, edge, nullptr)) return true;
        }

        // Check distance from triangle to cylinder axis
        const math::vec3 closest_on_tri = ClosestPoint(tri, cyl.center);
        return SquaredDistance(axis_seg, closest_on_tri) <= cyl.radius * cyl.radius;
    }

    bool Intersects(const Ellipsoid& a, const Aabb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Ellipsoid& a, const Cylinder& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Ellipsoid& other) noexcept
    {
        // Conservative approximation: check if closest points are within each other
        // or if centers are close enough based on radii

        // Simple distance check using maximum radii as approximation
        const math::vec3 delta = other.center - ellipsoid.center;
        const float dist_sq = math::dot(delta, delta);

        const float max_radius_a = math::utils::max(ellipsoid.radii[0],
                                                    math::utils::max(ellipsoid.radii[1], ellipsoid.radii[2]));
        const float max_radius_b = math::utils::max(other.radii[0],
                                                    math::utils::max(other.radii[1], other.radii[2]));
        const float sum_max = max_radius_a + max_radius_b;

        if (dist_sq > sum_max * sum_max) return false;

        // More precise test: check if closest point on one is inside the other
        const math::vec3 closest_on_a = ClosestPoint(ellipsoid, other.center);
        if (Contains(other, closest_on_a)) return true;

        const math::vec3 closest_on_b = ClosestPoint(other, ellipsoid.center);
        return Contains(ellipsoid, closest_on_b);
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Line& line, Result* result) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(ellipsoid.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 o = Rt * (line.point - ellipsoid.center);
        const math::vec3 d = Rt * line.direction;

        const math::vec3 os{o[0] / ellipsoid.radii[0], o[1] / ellipsoid.radii[1], o[2] / ellipsoid.radii[2]};
        const math::vec3 ds{d[0] / ellipsoid.radii[0], d[1] / ellipsoid.radii[1], d[2] / ellipsoid.radii[2]};

        const float a = math::dot(ds, ds);
        const float b = 2.0f * math::dot(os, ds);
        const float c = math::dot(os, os) - 1.0f;
        const float disc = b * b - 4 * a * c;
        if (disc < 0.0f) return false;
        const float root = math::utils::sqrt(disc);
        float t0 = (-b - root) / (2 * a);
        float t1 = (-b + root) / (2 * a);
        if (t0 > t1)
        {
            const float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        if (result)
        {
            result->t_min = t0;
            result->t_max = t1;
        }
        return true;
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Obb& obb) noexcept
    {
        // Find closest point on OBB to ellipsoid center
        const math::vec3 closest = ClosestPoint(obb, ellipsoid.center);

        // Check if closest point is inside ellipsoid or ellipsoid center is inside OBB
        return Contains(ellipsoid, closest) || Contains(obb, ellipsoid.center);
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Plane& plane) noexcept
    {
        // Transform plane normal to ellipsoid local space
        const math::mat3 R = math::utils::to_rotation_matrix(ellipsoid.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 local_normal = Rt * plane.normal;

        // Find the maximum extent of ellipsoid in the direction of plane normal
        // The farthest point on ellipsoid in direction n is at distance: sqrt(sum((r_i * n_i)^2))
        const math::vec3 scaled_normal{
            ellipsoid.radii[0] * local_normal[0],
            ellipsoid.radii[1] * local_normal[1],
            ellipsoid.radii[2] * local_normal[2]
        };
        const float max_extent = math::length(scaled_normal);

        // Signed distance from ellipsoid center to plane
        const float signed_dist = SignedDistance(plane, ellipsoid.center);

        // Ellipsoid intersects plane if signed distance is within max extent
        return math::utils::abs(signed_dist) <= max_extent;
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Ray& ray, Result* result) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(ellipsoid.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 o = Rt * (ray.origin - ellipsoid.center);
        const math::vec3 d = Rt * ray.direction;

        // scale by radii^-1
        const math::vec3 os{o[0] / ellipsoid.radii[0], o[1] / ellipsoid.radii[1], o[2] / ellipsoid.radii[2]};
        const math::vec3 ds{d[0] / ellipsoid.radii[0], d[1] / ellipsoid.radii[1], d[2] / ellipsoid.radii[2]};

        // Ray vs unit sphere
        const float a = math::dot(ds, ds);
        const float b = 2.0f * math::dot(os, ds);
        const float c = math::dot(os, os) - 1.0f;
        const float disc = b * b - 4 * a * c;
        if (disc < 0.0f) return false;
        const float root = math::utils::sqrt(disc);
        float t0 = (-b - root) / (2 * a);
        float t1 = (-b + root) / (2 * a);
        if (t0 > t1)
        {
            const float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        if (t1 < 0.0f) return false;
        if (result)
        {
            result->t_min = t0;
            result->t_max = t1;
        }
        return true;
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Segment& segment, Result* result) noexcept
    {
        Result tmp{};
        const Ray r{segment.start, Direction(segment)};
        if (!Intersects(ellipsoid, r, &tmp)) return false;
        float t0 = tmp.t_min, t1 = tmp.t_max;
        if (t1 < 0.0f || t0 > 1.0f) return false;
        if (result)
        {
            result->t_min = math::utils::max(0.0f, t0);
            result->t_max = math::utils::min(1.0f, t1);
        }
        return true;
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Sphere& sphere) noexcept
    {
        return SquaredDistance(ellipsoid, sphere.center) <= sphere.radius * sphere.radius;
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Triangle& triangle) noexcept
    {
        // Find closest point on triangle to ellipsoid center
        const math::vec3 closest = ClosestPoint(triangle, ellipsoid.center);

        // Check if closest point is inside ellipsoid
        return Contains(ellipsoid, closest);
    }

    bool Intersects(const Line& a, const Aabb& b, Result* result) noexcept
    {
        return Intersects(b, a, result);
    }

    bool Intersects(const Line& a, const Cylinder& b, Result* result) noexcept
    {
        return Intersects(b, a, result);
    }

    bool Intersects(const Line& a, const Ellipsoid& b, Result* result) noexcept
    {
        return Intersects(b, a, result);
    }

    bool Intersects(const Line& a, const Line& b, Result* result) noexcept
    {
        const math::vec3 w = a.point - b.point;
        const float a_dot = math::dot(a.direction, a.direction);
        const float b_dot = math::dot(b.direction, b.direction);
        const float ab_dot = math::dot(a.direction, b.direction);

        const float denom = a_dot * b_dot - ab_dot * ab_dot;

        // Check if lines are parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Lines are parallel, check if they're coincident
            const math::vec3 cross = math::cross(a.direction, w);
            const float cross_len_sq = math::dot(cross, cross);
            if (math::utils::nearly_equal(cross_len_sq, 0.0f, constants::INTERSECTION_EPSILON))
            {
                // Lines are coincident
                if (result) result->t = 0.0f;
                return true;
            }
            return false; // Parallel but not coincident
        }

        const float w_dot_a = math::dot(w, a.direction);
        const float w_dot_b = math::dot(w, b.direction);

        const float t_a = (ab_dot * w_dot_b - b_dot * w_dot_a) / denom;
        const float t_b = (a_dot * w_dot_b - ab_dot * w_dot_a) / denom;

        // Check if closest points are actually the same (lines intersect)
        const math::vec3 p_a = a.point + t_a * a.direction;
        const math::vec3 p_b = b.point + t_b * b.direction;
        const math::vec3 diff = p_a - p_b;

        if (math::dot(diff, diff) > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
        {
            return false; // Skew lines
        }

        if (result) result->t = t_a;
        return true;
    }

    bool Intersects(const Line& line, const Obb& obb, Result* result) noexcept
    {
        return Intersects(obb, line, result);
    }

    bool Intersects(const Line& line, const Plane& plane, Result* result) noexcept
    {
        const float denom = math::dot(plane.normal, line.direction);
        const float num = -(math::dot(plane.normal, line.point) + plane.distance);
        if (math::utils::nearly_equal(denom, 0.0f, constants::INTERSECTION_EPSILON)) return false;
        // parallel (coincident lines are rare; treat as no unique intersection)
        const float t = num / denom;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Line& line, const Ray& ray, Result* result) noexcept
    {
        Result tmp{};
        if (!Intersects(ray, line, &tmp)) return false;

        // Convert ray parameter to line parameter
        // We need to find t_line such that line.point + t_line * line.direction = ray.origin + tmp.t * ray.direction
        const math::vec3 intersection = ray.origin + tmp.t * ray.direction;
        const math::vec3 diff = intersection - line.point;
        const float t_line = math::dot(diff, line.direction) / math::dot(line.direction, line.direction);

        if (result) result->t = t_line;
        return true;
    }

    bool Intersects(const Line& line, const Segment& segment, Result* result) noexcept
    {
        const math::vec3 seg_dir = Direction(segment);
        const math::vec3 w = line.point - segment.start;

        const float a_dot = math::dot(line.direction, line.direction);
        const float b_dot = math::dot(seg_dir, seg_dir);
        const float ab_dot = math::dot(line.direction, seg_dir);

        const float denom = a_dot * b_dot - ab_dot * ab_dot;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            const math::vec3 cross = math::cross(line.direction, w);
            const float cross_len_sq = math::dot(cross, cross);
            if (math::utils::nearly_equal(cross_len_sq, 0.0f, constants::INTERSECTION_EPSILON))
            {
                if (result) result->t = 0.0f;
                return true;
            }
            return false;
        }

        const float w_dot_line = math::dot(w, line.direction);
        const float w_dot_seg = math::dot(w, seg_dir);

        const float t_line = (ab_dot * w_dot_seg - b_dot * w_dot_line) / denom;
        const float t_seg = (a_dot * w_dot_seg - ab_dot * w_dot_line) / denom;

        // Segment parameter must be in [0, 1]
        if (t_seg < 0.0f || t_seg > 1.0f) return false;

        // Check if points coincide
        const math::vec3 p_line = line.point + t_line * line.direction;
        const math::vec3 p_seg = segment.start + t_seg * seg_dir;
        const math::vec3 diff = p_line - p_seg;

        if (math::dot(diff, diff) > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        if (result) result->t = t_line;
        return true;
    }

    bool Intersects(const Line& line, const Sphere& sphere, Result* result) noexcept
    {
        const math::vec3 oc = line.point - sphere.center;
        const float a = math::dot(line.direction, line.direction);
        const float b = 2.0f * math::dot(oc, line.direction);
        const float c = math::dot(oc, oc) - sphere.radius * sphere.radius;
        const float disc = b * b - 4 * a * c;
        if (disc < 0.0f) return false;
        const float root = math::utils::sqrt(disc);
        float t0 = (-b - root) / (2 * a);
        float t1 = (-b + root) / (2 * a);
        if (t0 > t1)
        {
            const float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        if (result)
        {
            result->t_min = t0;
            result->t_max = t1;
        }
        return true;
    }

    bool Intersects(const Line& line, const Triangle& triangle, Result* result) noexcept
    {
        return Intersects(triangle, line, result);
    }

    bool Intersects(const Obb& a, const Aabb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Obb& a, const Cylinder& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Obb& a, const Ellipsoid& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Obb& obb, const Line& line, Result* result) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(obb.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 o = Rt * (line.point - obb.center);
        const math::vec3 d = Rt * line.direction;
        Result tmp{};
        const bool hit = Intersects(MakeAabbFromCenterExtent({0, 0, 0}, obb.half_sizes), Line{o, d}, &tmp);
        if (hit && result)
        {
            result->t_min = tmp.t_min;
            result->t_max = tmp.t_max;
        }
        return hit;
    }

    bool Intersects(const Obb& a, const Obb& b) noexcept
    {
        const math::mat3 A = math::utils::to_rotation_matrix(a.orientation); // localA->world
        const math::mat3 B = math::utils::to_rotation_matrix(b.orientation); // localB->world
        const math::mat3 R = transpose(A) * B; // localB in A-space TODO: verify!
        math::mat3 AbsR{};
        const float EPS = 1e-6f;
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) AbsR[i][j] = math::utils::abs(R[i][j]) + EPS;

        const math::vec3 aE = a.half_sizes;
        const math::vec3 bE = b.half_sizes;

        // T in A-space
        const math::vec3 T = transpose(A) * (b.center - a.center);

        // Test A's axes
        for (int i = 0; i < 3; ++i)
        {
            const float ra = aE[i];
            const float rb = bE[0] * AbsR[i][0] + bE[1] * AbsR[i][1] + bE[2] * AbsR[i][2];
            if (math::utils::abs(T[i]) > ra + rb) return false;
        }

        // Test B's axes
        for (int j = 0; j < 3; ++j)
        {
            const float ra = aE[0] * AbsR[0][j] + aE[1] * AbsR[1][j] + aE[2] * AbsR[2][j];
            const float rb = bE[j];
            const float t = math::utils::abs(T[0] * R[0][j] + T[1] * R[1][j] + T[2] * R[2][j]);
            if (t > ra + rb) return false;
        }

        // Test cross products A_i x B_j
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
            {
                const int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
                const int j1 = (j + 1) % 3, j2 = (j + 2) % 3;
                const float ra = aE[i1] * AbsR[i2][j] + aE[i2] * AbsR[i1][j];
                const float rb = bE[j1] * AbsR[i][j2] + bE[j2] * AbsR[i][j1];
                const float t = math::utils::abs(T[i2] * R[i1][j] - T[i1] * R[i2][j]);
                if (t > ra + rb) return false;
            }
        return true;
    }

    bool Intersects(const Obb& obb, const Plane& plane) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(obb.orientation);
        // projection radius of OBB onto plane normal
        math::vec3 an{
            math::utils::abs(plane.normal[0]), math::utils::abs(plane.normal[1]), math::utils::abs(plane.normal[2])
        };
        // Each world axis component projected onto local axes magnitude:
        // r = sum_i e_i * |n · axis_i|
        const math::vec3 e = obb.half_sizes;
        const float r =
            e[0] * math::utils::abs(R[0][0] * an[0] + R[1][0] * an[1] + R[2][0] * an[2]) +
            e[1] * math::utils::abs(R[0][1] * an[0] + R[1][1] * an[1] + R[2][1] * an[2]) +
            e[2] * math::utils::abs(R[0][2] * an[0] + R[1][2] * an[1] + R[2][2] * an[2]);
        const float s = math::dot(plane.normal, obb.center) + plane.distance;
        return math::utils::abs(s) <= r;
    }

    bool Intersects(const Obb& obb, const Ray& ray, Result* result) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(obb.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 o = Rt * (ray.origin - obb.center);
        const math::vec3 d = Rt * ray.direction;

        const Aabb a = MakeAabbFromCenterExtent(math::vec3{0, 0, 0}, obb.half_sizes);
        Result tmp{};
        const bool hit = Intersects(a, Ray{o, d}, &tmp);
        if (hit && result)
        {
            result->t_min = tmp.t_min;
            result->t_max = tmp.t_max;
        }
        return hit;
    }

    bool Intersects(const Obb& obb, const Segment& segment, Result* result) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(obb.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 s = Rt * (segment.start - obb.center);
        const math::vec3 e = Rt * (segment.end - obb.center);
        const Segment ls{s, e};
        Result tmp{};
        const bool hit = Intersects(MakeAabbFromCenterExtent({0, 0, 0}, obb.half_sizes), ls, &tmp);
        if (hit && result)
        {
            result->t_min = tmp.t_min;
            result->t_max = tmp.t_max;
        }
        return hit;
    }

    bool Intersects(const Obb& obb, const Sphere& sphere) noexcept
    {
        return SquaredDistance(obb, sphere.center) <= sphere.radius * sphere.radius;
    }

    bool Intersects(const Obb& obb, const Triangle& triangle) noexcept
    {
        // Transform triangle to OBB local space
        const math::mat3 R = math::utils::to_rotation_matrix(obb.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 v0 = Rt * (triangle.a - obb.center);
        const math::vec3 v1 = Rt * (triangle.b - obb.center);
        const math::vec3 v2 = Rt * (triangle.c - obb.center);

        // Test in OBB local space (equivalent to AABB test)
        const Aabb local_box = MakeAabbFromCenterExtent({0, 0, 0}, obb.half_sizes);
        const Triangle local_tri{v0, v1, v2};

        return Intersects(local_box, local_tri);
    }


    bool Intersects(const Plane& plane, const Aabb& aabb) noexcept
    {
        return Intersects(aabb, plane);
    }

    bool Intersects(const Plane& a, const Cylinder& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Plane& a, const Ellipsoid& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Plane& plane, const Line& line, Result* result) noexcept
    {
        return Intersects(line, plane, result);
    }

    bool Intersects(const Plane& a, const Obb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Plane& a, const Plane& b) noexcept
    {
        const float dot_product = math::dot(a.normal, b.normal);
        const float abs_dot = math::utils::abs(dot_product);

        // Check if parallel
        if (math::utils::nearly_equal(abs_dot, 1.0f, constants::PARALLEL_EPSILON))
        {
            // Parallel planes - check if coincident
            if (dot_product > 0.0f)
            {
                // Normals point in same direction
                return math::utils::nearly_equal(a.distance, b.distance, constants::INTERSECTION_EPSILON);
            }
            else
            {
                // Normals point in opposite directions
                return math::utils::nearly_equal(a.distance, -b.distance, constants::INTERSECTION_EPSILON);
            }
        }
        // Not parallel, they intersect in a line
        return true;
    }

    bool Intersects(const Plane& plane, const Ray& ray, Result* result) noexcept
    {
        return Intersects(ray, plane, result);
    }

    bool Intersects(const Plane& plane, const Segment& segment, Result* result) noexcept
    {
        return Intersects(segment, plane, result);
    }

    bool Intersects(const Plane& plane, const Sphere& sphere) noexcept
    {
        return math::utils::abs(math::dot(plane.normal, sphere.center) + plane.distance) <= sphere.radius;
    }

    bool Intersects(const Plane& a, const Triangle& b) noexcept
    {
        // Compute signed distances of all three vertices
        const float d0 = SignedDistance(a, b.a);
        const float d1 = SignedDistance(a, b.b);
        const float d2 = SignedDistance(a, b.c);

        // Find min and max distances
        const float min_d = math::utils::min(d0, math::utils::min(d1, d2));
        const float max_d = math::utils::max(d0, math::utils::max(d1, d2));

        // Intersects if vertices span across the plane (different signs) or touch it (zero)
        // If min and max have opposite signs: min * max < 0 (intersection)
        // If either is zero: min * max = 0 (vertex on plane)
        // If both same sign and non-zero: min * max > 0 (no intersection)
        return min_d * max_d <= 0.0f;
    }

    bool Intersects(const Ray& ray, const Aabb& box, Result* result) noexcept
    {
        return Intersects(box, ray, result);
    }

    bool Intersects(const Ray& a, const Cylinder& b, Result* result) noexcept
    {
        return Intersects(b, a, result);
    }

    bool Intersects(const Ray& ray, const Ellipsoid& ellipsoid, Result* result) noexcept
    {
        return Intersects(ellipsoid, ray, result);
    }

    bool Intersects(const Ray& ray, const Line& line, Result* result) noexcept
    {
        // Find closest points between ray and line
        const math::vec3 w0 = ray.origin - line.point;
        const float a = math::dot(ray.direction, ray.direction);
        const float b = math::dot(ray.direction, line.direction);
        const float c = math::dot(line.direction, line.direction);
        const float d = math::dot(ray.direction, w0);
        const float e = math::dot(line.direction, w0);

        const float denom = a * c - b * b;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Parallel - check if coincident
            const float dist_sq = math::length_squared(w0 - (e / c) * line.direction);
            if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
                return false;

            // Lines are coincident
            if (result) result->t = 0.0f;
            return true;
        }

        const float t_ray = (b * e - c * d) / denom;
        const float t_line = (a * e - b * d) / denom;

        // Ray requires t >= 0
        if (t_ray < 0.0f) return false;

        // Check if closest points actually coincide
        const math::vec3 p_ray = ray.origin + t_ray * ray.direction;
        const math::vec3 p_line = line.point + t_line * line.direction;
        const float dist_sq = math::length_squared(p_ray - p_line);

        if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
            return false;

        if (result) result->t = t_ray;
        return true;
    }

    bool Intersects(const Ray& ray, const Obb& obb, Result* result) noexcept
    {
        return Intersects(obb, ray, result);
    }

    bool Intersects(const Ray& ray, const Plane& plane, Result* result) noexcept
    {
        const float denom = math::dot(plane.normal, ray.direction);
        const float num = -(math::dot(plane.normal, ray.origin) + plane.distance);
        //TODO: check if this is the correct epsilon to use
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON)) return false;
        const float t = num / denom;
        if (t < 0.0f) return false;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Ray& ray, const Ray& other, Result* result) noexcept
    {
        // Find closest points between two rays
        const math::vec3 w0 = ray.origin - other.origin;
        const float a = math::dot(ray.direction, ray.direction);
        const float b = math::dot(ray.direction, other.direction);
        const float c = math::dot(other.direction, other.direction);
        const float d = math::dot(ray.direction, w0);
        const float e = math::dot(other.direction, w0);

        const float denom = a * c - b * b;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Parallel - check if coincident and same direction
            const float dist_sq = math::length_squared(w0 - (e / c) * other.direction);
            if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
                return false;

            // Check if rays point in same direction (not opposite)
            if (b < 0.0f) return false;

            if (result) result->t = 0.0f;
            return true;
        }

        const float t_a = (b * e - c * d) / denom;
        const float t_b = (a * e - b * d) / denom;

        // Both rays require t >= 0
        if (t_a < 0.0f || t_b < 0.0f) return false;

        // Check if closest points actually coincide
        const math::vec3 p_a = ray.origin + t_a * ray.direction;
        const math::vec3 p_b = other.origin + t_b * other.direction;
        const float dist_sq = math::length_squared(p_a - p_b);

        if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
            return false;

        if (result) result->t = t_a;
        return true;
    }

    bool Intersects(const Ray& ray, const Segment& segment, Result* result) noexcept
    {
        // Find closest points between ray and segment
        const math::vec3 seg_dir = Direction(segment);
        const math::vec3 w0 = ray.origin - segment.start;

        const float a = math::dot(ray.direction, ray.direction);
        const float b = math::dot(ray.direction, seg_dir);
        const float c = math::dot(seg_dir, seg_dir);
        const float d = math::dot(ray.direction, w0);
        const float e = math::dot(seg_dir, w0);

        const float denom = a * c - b * b;

        float t_ray, t_seg;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Parallel - check if coincident
            t_ray = 0.0f;
            t_seg = (b > c ? e / b : 0.0f);
        }
        else
        {
            t_ray = (b * e - c * d) / denom;
            t_seg = (a * e - b * d) / denom;
        }

        // Clamp segment parameter to [0, 1]
        t_seg = math::utils::clamp(t_seg, 0.0f, 1.0f);

        // Re-compute ray parameter for clamped segment point
        const math::vec3 seg_point = segment.start + t_seg * seg_dir;
        const math::vec3 diff = seg_point - ray.origin;
        t_ray = math::dot(diff, ray.direction) / a;

        // Ray requires t >= 0
        if (t_ray < 0.0f) return false;

        // Check if points actually coincide
        const math::vec3 p_ray = ray.origin + t_ray * ray.direction;
        const float dist_sq = math::length_squared(p_ray - seg_point);

        if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
            return false;

        if (result) result->t = t_ray;
        return true;
    }

    bool Intersects(const Ray& ray, const Sphere& sphere, Result* result) noexcept
    {
        const math::vec3 oc = ray.origin - sphere.center;
        const float a = math::dot(ray.direction, ray.direction);
        const float b = 2.0f * math::dot(oc, ray.direction);
        const float c = math::dot(oc, oc) - sphere.radius * sphere.radius;
        const float disc = b * b - 4 * a * c;
        if (disc < 0.0f) return false;
        const float root = math::utils::sqrt(disc);
        float t0 = (-b - root) / (2 * a);
        float t1 = (-b + root) / (2 * a);
        if (t0 > t1)
        {
            const float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        if (t1 < 0.0f) return false;
        if (result)
        {
            result->t_min = t0;
            result->t_max = t1;
        }
        return true;
    }

    bool Intersects(const Ray& ray, const Triangle& triangle, Result* result) noexcept
    {
        const float EPS = 1e-8f;
        const math::vec3 e1 = triangle.b - triangle.a;
        const math::vec3 e2 = triangle.c - triangle.a;
        const math::vec3 p = math::cross(ray.direction, e2);
        const float det = math::dot(e1, p);
        if (math::utils::abs(det) < EPS) return false;
        const float inv = 1.0f / det;
        const math::vec3 tvec = ray.origin - triangle.a;
        const float u = math::dot(tvec, p) * inv;
        if (u < 0.0f || u > 1.0f) return false;
        const math::vec3 q = math::cross(tvec, e1);
        const float v = math::dot(ray.direction, q) * inv;
        if (v < 0.0f || u + v > 1.0f) return false;
        const float t = math::dot(e2, q) * inv;
        if (t < 0.0f) return false;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Segment& segment, const Aabb& aabb, Result* result) noexcept
    {
        return Intersects(aabb, segment, result);
    }

    bool Intersects(const Segment& a, const Cylinder& b, Result* result) noexcept
    {
        return Intersects(b, a, result);
    }

    bool Intersects(const Segment& segment, const Ellipsoid& ellipsoid, Result* result) noexcept
    {
        return Intersects(ellipsoid, segment, result);
    }

    bool Intersects(const Segment& segment, const Line& line, Result* result) noexcept
    {
        const math::vec3 seg_dir = Direction(segment);
        const math::vec3 w0 = segment.start - line.point;

        const float a = math::dot(seg_dir, seg_dir);
        const float b = math::dot(seg_dir, line.direction);
        const float c = math::dot(line.direction, line.direction);
        const float d = math::dot(seg_dir, w0);
        const float e = math::dot(line.direction, w0);

        const float denom = a * c - b * b;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Check if coincident
            const math::vec3 cross = math::cross(seg_dir, w0);
            const float cross_len_sq = math::dot(cross, cross);
            if (math::utils::nearly_equal(cross_len_sq, 0.0f, constants::INTERSECTION_EPSILON))
            {
                if (result) result->t = 0.0f;
                return true;
            }
            return false;
        }

        const float t_seg = (b * e - c * d) / denom;
        const float t_line = (a * e - b * d) / denom;

        // Segment parameter must be in [0, 1]
        if (t_seg < 0.0f || t_seg > 1.0f) return false;

        // Check if points coincide
        const math::vec3 p_seg = segment.start + t_seg * seg_dir;
        const math::vec3 p_line = line.point + t_line * line.direction;
        const float dist_sq = math::length_squared(p_seg - p_line);

        if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        if (result) result->t = t_seg;
        return true;
    }

    bool Intersects(const Segment& segment, const Obb& obb, Result* result) noexcept
    {
        return Intersects(obb, segment, result);
    }

    bool Intersects(const Segment& segment, const Plane& plane, Result* result) noexcept
    {
        const math::vec3 d = Direction(segment);
        const float denom = math::dot(plane.normal, d);
        const float num = -(math::dot(plane.normal, segment.start) + plane.distance);
        //TODO: check if this is the correct epsilon to use
        if (math::utils::nearly_equal(denom, 0.0f, constants::SEPARATION_EPSILON)) return false;
        const float t = num / denom;
        if (t < 0.0f || t > 1.0f) return false;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Segment& segment, const Ray& ray, Result* result) noexcept
    {
        const math::vec3 seg_dir = Direction(segment);
        const math::vec3 w0 = segment.start - ray.origin;

        const float a = math::dot(seg_dir, seg_dir);
        const float b = math::dot(seg_dir, ray.direction);
        const float c = math::dot(ray.direction, ray.direction);
        const float d = math::dot(seg_dir, w0);
        const float e = math::dot(ray.direction, w0);

        const float denom = a * c - b * b;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Check if coincident
            const math::vec3 cross = math::cross(seg_dir, w0);
            const float cross_len_sq = math::dot(cross, cross);
            if (math::utils::nearly_equal(cross_len_sq, 0.0f, constants::INTERSECTION_EPSILON))
            {
                if (result) result->t = 0.0f;
                return true;
            }
            return false;
        }

        const float t_seg = (b * e - c * d) / denom;
        const float t_ray = (a * e - b * d) / denom;

        // Both must be valid
        if (t_seg < 0.0f || t_seg > 1.0f || t_ray < 0.0f) return false;

        // Check if points coincide
        const math::vec3 p_seg = segment.start + t_seg * seg_dir;
        const math::vec3 p_ray = ray.origin + t_ray * ray.direction;
        const float dist_sq = math::length_squared(p_seg - p_ray);

        if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        if (result) result->t = t_seg;
        return true;
    }

    bool Intersects(const Segment& segment, const Segment& other, Result* result) noexcept
    {
        const math::vec3 dir_a = Direction(segment);
        const math::vec3 dir_b = Direction(other);
        const math::vec3 w0 = segment.start - other.start;

        const float a = math::dot(dir_a, dir_a);
        const float b = math::dot(dir_a, dir_b);
        const float c = math::dot(dir_b, dir_b);
        const float d = math::dot(dir_a, w0);
        const float e = math::dot(dir_b, w0);

        const float denom = a * c - b * b;

        float t_a, t_b;

        // Check if parallel
        if (math::utils::nearly_equal(denom, 0.0f, constants::PARALLEL_EPSILON))
        {
            // Parallel segments - check if coincident
            const math::vec3 cross = math::cross(dir_a, w0);
            const float cross_len_sq = math::dot(cross, cross);
            if (math::utils::nearly_equal(cross_len_sq, 0.0f, constants::INTERSECTION_EPSILON))
            {
                // Coincident - they overlap if ranges overlap
                // Project onto one segment's direction
                const float proj_start_a = 0.0f;
                const float proj_end_a = math::dot(dir_a, dir_a);
                const float proj_start_b = math::dot(dir_b, w0);
                const float proj_end_b = proj_start_b + math::dot(dir_b, dir_a);

                const float overlap_min = math::utils::max(proj_start_a, math::utils::min(proj_start_b, proj_end_b));
                const float overlap_max = math::utils::min(proj_end_a, math::utils::max(proj_start_b, proj_end_b));

                if (overlap_min <= overlap_max)
                {
                    if (result) result->t = overlap_min / proj_end_a;
                    return true;
                }
            }
            return false;
        }

        t_a = (b * e - c * d) / denom;
        t_b = (a * e - b * d) / denom;

        // Clamp to valid ranges
        t_a = math::utils::clamp(t_a, 0.0f, 1.0f);
        t_b = math::utils::clamp(t_b, 0.0f, 1.0f);

        // Recompute for clamped values
        const math::vec3 p_a = segment.start + t_a * dir_a;
        const math::vec3 p_b = other.start + t_b * dir_b;
        const float dist_sq = math::length_squared(p_a - p_b);

        if (dist_sq > constants::INTERSECTION_EPSILON * constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        if (result) result->t = t_a;
        return true;
    }

    bool Intersects(const Segment& segment, const Sphere& sphere, Result* result) noexcept
    {
        Result tmp{};
        const Ray r{segment.start, Direction(segment)};
        if (!Intersects(r, sphere, &tmp)) return false;
        const float t0 = tmp.t_min, t1 = tmp.t_max;
        if (t1 < 0.0f || t0 > 1.0f) return false;
        if (result)
        {
            result->t_min = math::utils::max(0.0f, t0);
            result->t_max = math::utils::min(1.0f, t1);
        }
        return true;
    }

    bool Intersects(const Segment& segment, const Triangle& triangle, Result* result) noexcept
    {
        return Intersects(triangle, segment, result);
    }

    bool Intersects(const Sphere& sphere, const Aabb& aabb) noexcept
    {
        return Intersects(aabb, sphere);
    }

    bool Intersects(const Sphere& sphere, const Cylinder& cylinder) noexcept
    {
        return Intersects(cylinder, sphere);
    }

    bool Intersects(const Sphere& a, const Ellipsoid& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Sphere& sphere, const Line& line, Result* result) noexcept
    {
        return Intersects(line, sphere, result);
    }

    bool Intersects(const Sphere& sphere, const Obb& obb) noexcept
    {
        return Intersects(obb, sphere);
    }

    bool Intersects(const Sphere& sphere, const Plane& plane) noexcept
    {
        return Intersects(plane, sphere);
    }

    bool Intersects(const Sphere& sphere, const Ray& ray, Result* result) noexcept
    {
        return Intersects(ray, sphere, result);
    }

    bool Intersects(const Sphere& sphere, const Segment& segment, Result* result) noexcept
    {
        return Intersects(segment, sphere, result);
    }

    bool Intersects(const Sphere& a, const Sphere& b) noexcept
    {
        const math::vec3 diff = a.center - b.center;
        const float radius_sum = a.radius + b.radius;
        return math::dot(diff, diff) <= radius_sum * radius_sum;
    }

    bool Intersects(const Sphere& sphere, const Triangle& triangle) noexcept
    {
        return SquaredDistance(triangle, sphere.center) <= sphere.radius * sphere.radius;
    }

    bool Intersects(const Triangle& triangle, const Aabb& aabb) noexcept
    {
        return Intersects(aabb, triangle);
    }

    bool Intersects(const Triangle& a, const Cylinder& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Triangle& a, const Ellipsoid& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Triangle& triangle, const Line& line, Result* result) noexcept
    {
        // reuse ray test with both directions; accept any t on line
        const Ray r{line.point, line.direction};
        Result tmp{};
        if (!Intersects(r, triangle, &tmp)) return false;
        if (result) result->t = tmp.t;
        return true;
    }

    bool Intersects(const Triangle& a, const Obb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Triangle& a, const Plane& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Triangle& triangle, const Ray& ray, Result* result) noexcept
    {
        return Intersects(ray, triangle, result);
    }

    bool Intersects(const Triangle& triangle, const Segment& segment, Result* result) noexcept
    {
        Result tmp{};
        const Ray r{segment.start, Direction(segment)};
        if (!Intersects(r, triangle, &tmp)) return false;
        if (tmp.t < 0.0f || tmp.t > 1.0f) return false;
        if (result) result->t = tmp.t;
        return true;
    }

    bool Intersects(const Triangle& triangle, const Sphere& sphere) noexcept
    {
        return Intersects(sphere, triangle);
    }

    bool Intersects(const Triangle& a, const Triangle& b) noexcept
    {
        // Compute plane equations
        const math::vec3 n1 = math::cross(a.b - a.a, a.c - a.a);
        const math::vec3 n2 = math::cross(b.b - b.a, b.c - b.a);

        const float n1_len_sq = math::dot(n1, n1);
        const float n2_len_sq = math::dot(n2, n2);

        // Check for degenerate triangles
        if (n1_len_sq < constants::INTERSECTION_EPSILON || n2_len_sq < constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        // Check if triangle b's vertices are on opposite sides of triangle a's plane
        const float d1_a = math::dot(n1, b.a - a.a);
        const float d1_b = math::dot(n1, b.b - a.a);
        const float d1_c = math::dot(n1, b.c - a.a);

        const float d1_min = math::utils::min(d1_a, math::utils::min(d1_b, d1_c));
        const float d1_max = math::utils::max(d1_a, math::utils::max(d1_b, d1_c));

        // Early rejection if all vertices on same side
        if (d1_min > constants::INTERSECTION_EPSILON || d1_max < -constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        // Check if triangle a's vertices are on opposite sides of triangle b's plane
        const float d2_a = math::dot(n2, a.a - b.a);
        const float d2_b = math::dot(n2, a.b - b.a);
        const float d2_c = math::dot(n2, a.c - b.a);

        const float d2_min = math::utils::min(d2_a, math::utils::min(d2_b, d2_c));
        const float d2_max = math::utils::max(d2_a, math::utils::max(d2_b, d2_c));

        // Early rejection if all vertices on same side
        if (d2_min > constants::INTERSECTION_EPSILON || d2_max < -constants::INTERSECTION_EPSILON)
        {
            return false;
        }

        // Check if coplanar
        const float abs_d1_min = math::utils::abs(d1_min);
        const float abs_d1_max = math::utils::abs(d1_max);
        const bool coplanar = (abs_d1_min < constants::INTERSECTION_EPSILON &&
            abs_d1_max < constants::INTERSECTION_EPSILON);

        if (coplanar)
        {
            // Use 2D overlap test - project to dominant axis plane
            const math::vec3 abs_n1{math::utils::abs(n1[0]), math::utils::abs(n1[1]), math::utils::abs(n1[2])};
            std::size_t dominant_axis = 2;
            if (abs_n1[0] > abs_n1[1] && abs_n1[0] > abs_n1[2])
            {
                dominant_axis = 0;
            }
            else if (abs_n1[1] > abs_n1[2])
            {
                dominant_axis = 1;
            }

            auto project_to_2d = [dominant_axis](const math::vec3& p) -> math::vec2
            {
                switch (dominant_axis)
                {
                case 0: return {p[1], p[2]};
                case 1: return {p[0], p[2]};
                default: return {p[0], p[1]};
                }
            };

            const std::array<math::vec2, 3> tri_a{project_to_2d(a.a), project_to_2d(a.b), project_to_2d(a.c)};
            const std::array<math::vec2, 3> tri_b{project_to_2d(b.a), project_to_2d(b.b), project_to_2d(b.c)};

            // Check edge intersections
            auto segments_intersect_2d = [](const math::vec2& p0, const math::vec2& p1,
                                            const math::vec2& q0, const math::vec2& q1) -> bool
            {
                auto cross_2d = [](const math::vec2& a, const math::vec2& b) -> float
                {
                    return a[0] * b[1] - a[1] * b[0];
                };

                const math::vec2 r = p1 - p0;
                const math::vec2 s = q1 - q0;
                const float rxs = cross_2d(r, s);
                const math::vec2 qp = q0 - p0;
                const float qpxr = cross_2d(qp, r);

                if (math::utils::abs(rxs) < constants::INTERSECTION_EPSILON)
                {
                    return math::utils::abs(qpxr) < constants::INTERSECTION_EPSILON;
                }

                const float t = cross_2d(qp, s) / rxs;
                const float u = qpxr / rxs;

                return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
            };

            // Check all edge pairs
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (segments_intersect_2d(tri_a[i], tri_a[(i + 1) % 3], tri_b[j], tri_b[(j + 1) % 3]))
                    {
                        return true;
                    }
                }
            }

            // Check if any vertex is inside the other triangle
            auto point_in_triangle_2d = [](const std::array<math::vec2, 3>& tri, const math::vec2& p) -> bool
            {
                auto sign = [](const math::vec2& p1, const math::vec2& p2, const math::vec2& p3) -> float
                {
                    return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
                };

                const float d1 = sign(p, tri[0], tri[1]);
                const float d2 = sign(p, tri[1], tri[2]);
                const float d3 = sign(p, tri[2], tri[0]);

                const bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
                const bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

                return !(has_neg && has_pos);
            };

            for (const auto& vertex : tri_a)
            {
                if (point_in_triangle_2d(tri_b, vertex)) return true;
            }
            for (const auto& vertex : tri_b)
            {
                if (point_in_triangle_2d(tri_a, vertex)) return true;
            }

            return false;
        }

        // Non-coplanar: Check if edges of one triangle intersect the other
        const std::array<Segment, 3> edges_a{
            Segment{a.a, a.b},
            Segment{a.b, a.c},
            Segment{a.c, a.a}
        };

        const std::array<Segment, 3> edges_b{
            Segment{b.a, b.b},
            Segment{b.b, b.c},
            Segment{b.c, b.a}
        };

        for (const auto& edge : edges_a)
        {
            if (Intersects(edge, b, nullptr)) return true;
        }

        for (const auto& edge : edges_b)
        {
            if (Intersects(edge, a, nullptr)) return true;
        }

        return false;
    }

    //------------------------------------------------------------------------------------------------------------------

    bool Contains(const Aabb& outer, const math::vec3& inner) noexcept
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (inner[i] < outer.min[i] || inner[i] > outer.max[i])
                return false;
        }
        return true;
    }

    bool Contains(const Aabb& outer, const Aabb& inner) noexcept
    {
        // More efficient than testing each vertex: check bounds directly
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (inner.min[i] < outer.min[i] || inner.max[i] > outer.max[i])
                return false;
        }
        return true;
    }

    bool Contains(const Aabb& outer, const Cylinder& inner) noexcept
    {
        // Check cylinder end cap centers
        const math::vec3 top = TopCenter(inner);
        const math::vec3 bottom = BottomCenter(inner);
        if (!Contains(outer, top) || !Contains(outer, bottom)) return false;

        // Check points on the circular edges
        const math::vec3 axis = AxisDirection(inner);
        math::vec3 perp1, perp2;
        if (math::utils::abs(axis[0]) < 0.9f)
        {
            perp1 = math::normalize(math::cross(axis, math::vec3{1, 0, 0}));
        }
        else
        {
            perp1 = math::normalize(math::cross(axis, math::vec3{0, 1, 0}));
        }
        perp2 = math::cross(axis, perp1);

        for (int i = 0; i < 8; ++i)
        {
            const float angle = i * 3.14159265f / 4.0f;
            const math::vec3 offset = inner.radius * (math::utils::cos(angle) * perp1 + math::utils::sin(angle) *
                perp2);
            if (!Contains(outer, top + offset)) return false;
            if (!Contains(outer, bottom + offset)) return false;
        }
        return true;
    }

    bool Contains(const Aabb& outer, const Ellipsoid& inner) noexcept
    {
        // Transform ellipsoid to axis-aligned and test bounding box
        // This is conservative - we test if the ellipsoid's oriented bounding box fits

        const math::mat3 R = math::utils::to_rotation_matrix(inner.orientation);
        const math::vec3 radii = inner.radii;

        // Find axis-aligned bounding box of the oriented ellipsoid
        // Max extent in each world axis direction
        math::vec3 half_extents{0.0f, 0.0f, 0.0f};

        for (std::size_t i = 0; i < 3; ++i)
        {
            // Sum of absolute values of transformed radii in this direction
            half_extents[i] =
                math::utils::abs(R[0][i]) * radii[0] +
                math::utils::abs(R[1][i]) * radii[1] +
                math::utils::abs(R[2][i]) * radii[2];
        }

        const math::vec3 ellipsoid_min = inner.center - half_extents;
        const math::vec3 ellipsoid_max = inner.center + half_extents;

        // Check if this bounding box fits in outer AABB
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (ellipsoid_min[i] < outer.min[i] || ellipsoid_max[i] > outer.max[i])
                return false;
        }

        return true;
    }

    bool Contains(const Aabb& outer, const Obb& inner) noexcept
    {
        // Test all 8 corners of the OBB
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner))
                return false;
        }
        return true;
    }

    bool Contains(const Aabb& outer, const Segment& inner) noexcept
    {
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Aabb& outer, const Sphere& inner) noexcept
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (inner.center[i] - inner.radius < outer.min[i]) return false;
            if (inner.center[i] + inner.radius > outer.max[i]) return false;
        }
        return true;
    }

    namespace
    {
        struct CylinderPointInfo
        {
            float axial_distance; // signed distance along axis
            float radial_distance_sq; // squared distance from axis
        };

        CylinderPointInfo GetCylinderPointInfo(const Cylinder& cylinder, const math::vec3& point) noexcept
        {
            const math::vec3 axis_dir = AxisDirection(cylinder);
            const math::vec3 delta = point - cylinder.center;
            const float axial = math::dot(delta, axis_dir);
            const math::vec3 radial_vec = delta - axial * axis_dir;
            const float radial_sq = math::dot(radial_vec, radial_vec);
            return {axial, radial_sq};
        }
    }

    bool Contains(const Aabb& outer, const Triangle& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Contains(const Cylinder& outer, const math::vec3& inner) noexcept
    {
        const auto info = GetCylinderPointInfo(outer, inner);
        if (math::utils::abs(info.axial_distance) > outer.half_height) return false;
        return info.radial_distance_sq <= outer.radius * outer.radius;
    }

    bool Contains(const Cylinder& outer, const Aabb& inner) noexcept
    {
        // Check all 8 corners of the AABB
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner)) return false;
        }
        return true;
    }

    bool Contains(const Cylinder& outer, const Cylinder& inner) noexcept
    {
        const math::vec3 outer_axis = AxisDirection(outer);
        const math::vec3 inner_axis = AxisDirection(inner);

        // Check if axes are parallel
        const float dot = math::dot(outer_axis, inner_axis);
        const bool parallel = math::utils::abs(math::utils::abs(dot) - 1.0f) < constants::PARALLEL_EPSILON;

        if (!parallel)
        {
            // Non-parallel cylinders - check endpoints and sample points
            const math::vec3 inner_top = TopCenter(inner);
            const math::vec3 inner_bottom = BottomCenter(inner);

            // Check if both end centers are contained
            if (!Contains(outer, inner_top) || !Contains(outer, inner_bottom)) return false;

            // Check edge points of the inner cylinder's end caps
            math::vec3 perp1, perp2;
            if (math::utils::abs(inner_axis[0]) < 0.9f)
            {
                perp1 = math::normalize(math::cross(inner_axis, math::vec3{1, 0, 0}));
            }
            else
            {
                perp1 = math::normalize(math::cross(inner_axis, math::vec3{0, 1, 0}));
            }
            perp2 = math::cross(inner_axis, perp1);

            for (int i = 0; i < 8; ++i)
            {
                const float angle = i * 3.14159265f / 4.0f;
                const math::vec3 offset = inner.radius * (math::utils::cos(angle) * perp1 + math::utils::sin(angle) *
                    perp2);
                if (!Contains(outer, inner_top + offset)) return false;
                if (!Contains(outer, inner_bottom + offset)) return false;
            }
            return true;
        }

        // Parallel cylinders
        const math::vec3 center_delta = inner.center - outer.center;
        const float axial = math::dot(center_delta, outer_axis);
        const math::vec3 radial = center_delta - axial * outer_axis;
        const float radial_dist = math::length(radial);

        // Check radial containment
        if (radial_dist + inner.radius > outer.radius) return false;

        // Check axial containment
        const float inner_extent = math::utils::abs(axial) + inner.half_height;
        return inner_extent <= outer.half_height;
    }

    bool Contains(const Cylinder& outer, const Ellipsoid& inner) noexcept
    {
        //TODO: more efficient algorithm
        // Conservative approach: sample points on the ellipsoid surface
        const math::mat3 R = math::utils::to_rotation_matrix(inner.orientation);
        const int samples = 16;

        for (int i = 0; i < samples; ++i)
        {
            const float theta = 2.0f * 3.14159265f * i / samples;
            for (int j = 0; j < samples / 2; ++j)
            {
                const float phi = 3.14159265f * j / (samples / 2 - 1);
                const math::vec3 local{
                    inner.radii[0] * math::utils::sin(phi) * math::utils::cos(theta),
                    inner.radii[1] * math::utils::sin(phi) * math::utils::sin(theta),
                    inner.radii[2] * math::utils::cos(phi)
                };
                const math::vec3 world = inner.center + R * local;
                if (!Contains(outer, world)) return false;
            }
        }
        return true;
    }

    bool Contains(const Cylinder& outer, const Obb& inner) noexcept
    {
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner)) return false;
        }
        return true;
    }

    bool Contains(const Cylinder& outer, const Segment& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Cylinder& outer, const Sphere& inner) noexcept
    {
        const math::vec3 axis_dir = AxisDirection(outer);
        const float axis_len_sq = math::length_squared(axis_dir);
        const math::vec3 delta = inner.center - outer.center;

        //TODO: check if this is the correct epsilon to use
        if (math::utils::nearly_equal(axis_len_sq, 0.0f, constants::PARALLEL_EPSILON))
        {
            if (inner.radius > outer.radius || inner.radius > outer.half_height) return false;
            const float max_allow = outer.radius - inner.radius;
            if (max_allow < 0.0f) return false;
            return math::dot(delta, delta) <= max_allow * max_allow;
        }

        const float axial = math::dot(delta, axis_dir);
        const float axial_allow = outer.half_height - inner.radius;
        if (axial_allow < 0.0f || math::utils::abs(axial) > axial_allow) return false;

        const math::vec3 radial_vec = delta - axial * axis_dir;
        const float radial_sq = math::dot(radial_vec, radial_vec);
        const float radial_allow = outer.radius - inner.radius;
        if (radial_allow < 0.0f) return false;

        return radial_sq <= radial_allow * radial_allow;
    }

    bool Contains(const Cylinder& outer, const Triangle& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    // Map world -> ellipsoid local: x' = R^T (x - c); then scale by radii^-1
    static inline math::vec3 EllipsoidToUnit(const Ellipsoid& ellipsoid, const math::vec3& point) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(ellipsoid.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 q = Rt * (point - ellipsoid.center);
        return math::vec3{q[0] / ellipsoid.radii[0], q[1] / ellipsoid.radii[1], q[2] / ellipsoid.radii[2]};
    }

    bool Contains(const Ellipsoid& outer, const math::vec3& inner) noexcept
    {
        const math::vec3 u = EllipsoidToUnit(outer, inner);
        return math::dot(u, u) <= 1.0f;
    }

    bool Contains(const Ellipsoid& outer, const Aabb& inner) noexcept
    {
        // Check all 8 corners of AABB
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner))
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Ellipsoid& outer, const Cylinder& inner) noexcept
    {
        // Sample points on cylinder surface
        const math::vec3 axis = AxisDirection(inner);
        math::vec3 perp1, perp2;
        if (math::utils::abs(axis[0]) < 0.9f)
        {
            perp1 = math::normalize(math::cross(axis, math::vec3{1, 0, 0}));
        }
        else
        {
            perp1 = math::normalize(math::cross(axis, math::vec3{0, 1, 0}));
        }
        perp2 = math::cross(axis, perp1);

        const int angle_samples = 16;
        const int height_samples = 8;

        for (int i = 0; i < angle_samples; ++i)
        {
            const float angle = 2.0f * 3.14159265f * i / angle_samples;
            const math::vec3 radial = inner.radius * (math::utils::cos(angle) * perp1 + math::utils::sin(angle) *
                perp2);

            for (int j = 0; j < height_samples; ++j)
            {
                const float t = -1.0f + 2.0f * j / (height_samples - 1);
                const math::vec3 point = inner.center + t * inner.half_height * axis + radial;
                if (!Contains(outer, point)) return false;
            }
        }

        // Check end caps
        const math::vec3 top = TopCenter(inner);
        const math::vec3 bottom = BottomCenter(inner);
        if (!Contains(outer, top) || !Contains(outer, bottom)) return false;

        return true;
    }

    bool Contains(const Ellipsoid& outer, const Ellipsoid& inner) noexcept
    {
        // Conservative test: check if all extreme points of inner ellipsoid are inside outer
        const math::mat3 R = math::utils::to_rotation_matrix(inner.orientation);

        for (int i = 0; i < 3; ++i)
        {
            const math::vec3 axis{R[0][i], R[1][i], R[2][i]};
            const math::vec3 extent_pos = inner.center + axis * inner.radii[i];
            const math::vec3 extent_neg = inner.center - axis * inner.radii[i];

            if (!Contains(outer, extent_pos) || !Contains(outer, extent_neg))
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Ellipsoid& outer, const Obb& inner) noexcept
    {
        // Check all 8 corners of OBB
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner))
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Ellipsoid& outer, const Segment& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Ellipsoid& outer, const Sphere& inner) noexcept
    {
        const math::vec3 radii = outer.radii;
        if (radii[0] <= 0.0f || radii[1] <= 0.0f || radii[2] <= 0.0f)
        {
            if (inner.radius > 0.0f) return false;
            const math::vec3 unit = EllipsoidToUnit(outer, inner.center);
            //TODO: check if this is the correct epsilon to use
            return math::utils::nearly_equal(math::length_squared(unit), 0.0f, constants::PARALLEL_EPSILON);
        }

        const math::mat3 R = math::utils::to_rotation_matrix(outer.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 local = Rt * (inner.center - outer.center);

        const math::vec3 scaled{local[0] / radii[0], local[1] / radii[1], local[2] / radii[2]};
        const float scaled_len = math::length(scaled);
        if (scaled_len >= 1.0f) return false; // center is outside or on boundary

        if (math::utils::nearly_equal(math::length_squared(local), 0.0f, constants::PARALLEL_EPSILON))
        {
            const float min_radius = math::utils::min(radii[0], math::utils::min(radii[1], radii[2]));
            return inner.radius <= min_radius;
        }

        const float local_len = math::length(local);
        const float t = 1.0f / scaled_len;
        const float clearance = local_len * (t - 1.0f);
        return clearance >= inner.radius;
    }

    bool Contains(const Ellipsoid& outer, const Triangle& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Contains(const Obb& outer, const math::vec3& inner) noexcept
    {
        // Transform point to OBB local space
        const math::mat3 R = math::utils::to_rotation_matrix(outer.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 local = Rt * (inner - outer.center);

        // Check if point is within half-sizes in all dimensions
        for (std::size_t i = 0; i < 3; ++i)
        {
            if (math::utils::abs(local[i]) > outer.half_sizes[i])
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Obb& outer, const Aabb& inner) noexcept
    {
        // Check all 8 corners of AABB
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner))
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Obb& outer, const Cylinder& inner) noexcept
    {
        // Sample points on cylinder surface
        const math::vec3 axis = AxisDirection(inner);
        math::vec3 perp1, perp2;
        if (math::utils::abs(axis[0]) < 0.9f)
        {
            perp1 = math::normalize(math::cross(axis, math::vec3{1, 0, 0}));
        }
        else
        {
            perp1 = math::normalize(math::cross(axis, math::vec3{0, 1, 0}));
        }
        perp2 = math::cross(axis, perp1);

        const math::vec3 top = TopCenter(inner);
        const math::vec3 bottom = BottomCenter(inner);

        for (int i = 0; i < 8; ++i)
        {
            const float angle = i * 3.14159265f / 4.0f;
            const math::vec3 offset = inner.radius * (math::utils::cos(angle) * perp1 + math::utils::sin(angle) *
                perp2);
            if (!Contains(outer, top + offset)) return false;
            if (!Contains(outer, bottom + offset)) return false;
        }

        return true;
    }

    bool Contains(const Obb& outer, const Ellipsoid& inner) noexcept
    {
        // Check extreme points of ellipsoid along its principal axes
        const math::mat3 R = math::utils::to_rotation_matrix(inner.orientation);

        for (int i = 0; i < 3; ++i)
        {
            const math::vec3 axis{R[0][i], R[1][i], R[2][i]};
            const math::vec3 extent_pos = inner.center + axis * inner.radii[i];
            const math::vec3 extent_neg = inner.center - axis * inner.radii[i];

            if (!Contains(outer, extent_pos) || !Contains(outer, extent_neg))
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Obb& outer, const Obb& inner) noexcept
    {
        // Check all 8 corners of inner OBB
        const auto corners = GetCorners(inner);
        for (const auto& corner : corners)
        {
            if (!Contains(outer, corner))
            {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Obb& outer, const Segment& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Obb& outer, const Sphere& inner) noexcept
    {
        const math::mat3 R = math::utils::to_rotation_matrix(outer.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 local = Rt * (inner.center - outer.center);
        for (std::size_t i = 0; i < 3; ++i)
        {
            const float limit = outer.half_sizes[i] - inner.radius;
            if (limit < 0.0f) return false;
            if (math::utils::abs(local[i]) > limit) return false;
        }
        return true;
    }

    bool Contains(const Obb& outer, const Triangle& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Contains(const Plane& outer, const math::vec3& inner, float eps) noexcept
    {
        const float dist = math::utils::abs(SignedDistance(outer, inner));
        return dist <= eps;
    }

    bool Contains(const Plane& outer, const Line& inner, float eps) noexcept
    {
        // Line is contained if its origin is on the plane and direction is perpendicular to normal
        if (!Contains(outer, inner.point, eps)) return false;

        const float dot = math::utils::abs(math::dot(outer.normal, inner.direction));
        return dot <= eps;
    }

    bool Contains(const Plane& outer, const Plane& inner, float eps) noexcept
    {
        // Planes are the same if normals are parallel and distances equal
        const float dot = math::dot(outer.normal, inner.normal);
        const float abs_dot = math::utils::abs(dot);

        // Check if normals are parallel
        if (math::utils::abs(abs_dot - 1.0f) > eps) return false;

        // Check if distances match (accounting for normal direction)
        if (dot > 0.0f)
        {
            return math::utils::abs(outer.distance - inner.distance) <= eps;
        }
        else
        {
            return math::utils::abs(outer.distance + inner.distance) <= eps;
        }
    }

    bool Contains(const Plane& outer, const Ray& inner, float eps) noexcept
    {
        // Ray is contained if origin is on plane and direction is perpendicular to normal
        if (!Contains(outer, inner.origin, eps)) return false;

        const float dot = math::utils::abs(math::dot(outer.normal, inner.direction));
        return dot <= eps;
    }

    bool Contains(const Plane& outer, const Segment& inner, float eps) noexcept
    {
        return Contains(outer, inner.start, eps) && Contains(outer, inner.end, eps);
    }

    bool Contains(const Plane& outer, const Triangle& inner, float eps) noexcept
    {
        return Contains(outer, inner.a, eps) &&
            Contains(outer, inner.b, eps) &&
            Contains(outer, inner.c, eps);
    }

    bool Contains(const Sphere& outer, const math::vec3& inner) noexcept
    {
        return math::length_squared(inner - outer.center) <= outer.radius * outer.radius;
    }

    bool Contains(const Sphere& outer, const Aabb& inner) noexcept
    {
        const math::vec3 diff_min = inner.min - outer.center;
        const math::vec3 diff_max = inner.max - outer.center;
        math::vec3 farthest{};
        for (std::size_t i = 0; i < 3; ++i)
        {
            farthest[i] = (math::utils::abs(diff_min[i]) > math::utils::abs(diff_max[i])) ? inner.min[i] : inner.max[i];
        }
        const math::vec3 diff = farthest - outer.center;
        return math::dot(diff, diff) <= outer.radius * outer.radius;
    }

    bool Contains(const Sphere& outer, const Cylinder& inner) noexcept
    {
        const math::vec3 axis_dir = AxisDirection(inner);
        const float axis_len_sq = math::length_squared(axis_dir);
        const math::vec3 delta = inner.center - outer.center;

        //TODO: check if this is the correct epsilon to use
        if (math::utils::nearly_equal(axis_len_sq, 0.0f, constants::PARALLEL_EPSILON))
        {
            const float radial = math::length(delta) + inner.radius;
            return radial <= outer.radius;
        }

        const float parallel_center = math::dot(delta, axis_dir);
        const math::vec3 perp = delta - parallel_center * axis_dir;
        const float perp_len = math::length(perp);

        float max_dist_sq = 0.0f;
        for (const float t : {inner.half_height, -inner.half_height})
        {
            const float parallel = parallel_center + t;
            const float radial = perp_len + inner.radius;
            const float dist_sq = parallel * parallel + radial * radial;
            if (dist_sq > max_dist_sq) max_dist_sq = dist_sq;
        }

        return max_dist_sq <= outer.radius * outer.radius;
    }

    bool Contains(const Sphere& outer, const Ellipsoid& inner) noexcept
    {
        //TODO: optimize? too complex for real nodes?
        const math::mat3 R = math::utils::to_rotation_matrix(inner.orientation);
        const math::mat3 Rt = transpose(R);
        const math::vec3 radii = inner.radii;

        const math::vec3 d = Rt * (inner.center - outer.center);
        if (radii[0] <= 0.0f || radii[1] <= 0.0f || radii[2] <= 0.0f)
        {
            const math::vec3 diff = inner.center - outer.center;
            return math::dot(diff, diff) <= outer.radius * outer.radius;
        }

        const float r0 = radii[0];
        const float r1 = radii[1];
        const float r2 = radii[2];

        const float max_r_sq = math::utils::max(r0 * r0, math::utils::max(r1 * r1, r2 * r2));

        auto evaluate = [&](double lambda) noexcept
        {
            double sum = 0.0;
            const double values[3] = {d[0], d[1], d[2]};
            const double radii_sq[3] = {r0 * r0, r1 * r1, r2 * r2};
            for (int i = 0; i < 3; ++i)
            {
                const double denom = lambda - radii_sq[i];
                const double numer = radii_sq[i] * values[i];
                sum += (numer * numer) / (denom * denom);
            }
            return sum - 1.0;
        };

        const float diff_len_sq = math::dot(inner.center - outer.center, inner.center - outer.center);
        //TODO: check if this is the correct epsilon to use
        if (math::utils::nearly_equal(diff_len_sq, 0.0f, constants::PARALLEL_EPSILON))
        {
            const float max_radius = math::utils::max(r0, math::utils::max(r1, r2));
            return max_radius <= outer.radius;
        }

        double low = static_cast<double>(max_r_sq) + 1e-6;
        double high = low;
        while (evaluate(high) > 0.0)
        {
            high *= 2.0;
        }

        for (int iter = 0; iter < 64; ++iter)
        {
            const double mid = 0.5 * (low + high);
            const double value = evaluate(mid);
            if (value > 0.0)
            {
                low = mid;
            }
            else
            {
                high = mid;
            }
        }
        const double lambda = high;

        const double values[3] = {d[0], d[1], d[2]};
        const double radii_sq[3] = {r0 * r0, r1 * r1, r2 * r2};
        math::vec3 y{};
        for (int i = 0; i < 3; ++i)
        {
            const double denom = lambda - radii_sq[i];
            y[i] = static_cast<float>(radii_sq[i] * values[i] / denom);
        }

        const math::vec3 local_point = d + math::vec3{y[0], y[1], y[2]};
        const float max_dist_sq = math::dot(local_point, local_point);
        return max_dist_sq <= outer.radius * outer.radius;
    }

    bool Contains(const Sphere& outer, const Obb& inner) noexcept
    {
        const auto corners = GetCorners(inner);
        const float radius_sq = outer.radius * outer.radius;
        for (const auto& corner : corners)
        {
            const math::vec3 diff = corner - outer.center;
            if (math::dot(diff, diff) > radius_sq) return false;
        }
        return true;
    }

    bool Contains(const Sphere& outer, const Segment& inner) noexcept
    {
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Sphere& outer, const Sphere& inner) noexcept
    {
        if (inner.radius > outer.radius) return false;
        const float radius_diff = outer.radius - inner.radius;
        const math::vec3 diff = inner.center - outer.center;
        return math::dot(diff, diff) <= radius_diff * radius_diff;
    }

    bool Contains(const Sphere& outer, const Triangle& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Contains(const Triangle& outer, const math::vec3& inner) noexcept
    {
        // Compute barycentric coordinates
        const math::vec3 v0 = outer.b - outer.a;
        const math::vec3 v1 = outer.c - outer.a;
        const math::vec3 v2 = inner - outer.a;

        const float d00 = math::dot(v0, v0);
        const float d01 = math::dot(v0, v1);
        const float d11 = math::dot(v1, v1);
        const float d20 = math::dot(v2, v0);
        const float d21 = math::dot(v2, v1);

        const float denom = d00 * d11 - d01 * d01;
        if (math::utils::abs(denom) < constants::INTERSECTION_EPSILON) return false;

        const float v = (d11 * d20 - d01 * d21) / denom;
        const float w = (d00 * d21 - d01 * d20) / denom;
        const float u = 1.0f - v - w;

        // Point is inside if all barycentric coordinates are non-negative
        const float eps = constants::INTERSECTION_EPSILON;
        return (u >= -eps && v >= -eps && w >= -eps);
    }

    bool Contains(const Triangle& outer, const Triangle& inner) noexcept
    {
        // Triangle contains another if all vertices are inside
        return Contains(outer, inner.a) &&
            Contains(outer, inner.b) &&
            Contains(outer, inner.c);
    }

    bool Contains(const Triangle& outer, const Segment& inner) noexcept
    {
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }
} // namespace engine::geometry
