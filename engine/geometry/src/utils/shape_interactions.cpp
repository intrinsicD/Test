#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/shapes.hpp"

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
        //TODO
    }

    bool Intersects(const Aabb& a, const Ellipsoid& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Aabb& box, const Line& ln, Result<Aabb, Line>* result) noexcept
    {
        const float INF = std::numeric_limits<float>::infinity();
        float tmin = -INF, tmax = +INF;

        for (std::size_t i = 0; i < 3; ++i)
        {
            const float o = ln.point[i];
            const float d = ln.direction[i];
            if (d == 0.0f)
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
        const math::mat3 B = obb.orientation.to_rotation_matrix(); // localB->world
        math::mat3 R = B; // since RA=I
        math::mat3 AbsR{};
        const float EPS = 1e-6f;
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) AbsR[i][j] = abs(R[i][j]) + EPS;

        const math::vec3 aC = Center(aabb);
        const math::vec3 aE = Extent(aabb);
        const math::vec3 bE = obb.half_sizes;
        const math::vec3 T = (obb.center - aC); // already in A-space (world)

        // A's axes
        for (int i = 0; i < 3; ++i)
        {
            const float ra = aE[i];
            const float rb = bE[0] * AbsR[i][0] + bE[1] * AbsR[i][1] + bE[2] * AbsR[i][2];
            if (abs(T[i]) > ra + rb) return false;
        }
        // B's axes
        for (int j = 0; j < 3; ++j)
        {
            const float ra = aE[0] * AbsR[0][j] + aE[1] * AbsR[1][j] + aE[2] * AbsR[2][j];
            const float rb = bE[j];
            const float t = abs(T[0] * R[0][j] + T[1] * R[1][j] + T[2] * R[2][j]);
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
                const float t = abs(T[i2] * R[i1][j] - T[i1] * R[i2][j]);
                if (t > ra + rb) return false;
            }
        return true;
    }

    bool Intersects(const Aabb& aabb, const Plane& plane) noexcept
    {
        const math::vec3 c = Center(aabb);
        const math::vec3 e = Extent(aabb);
        const float s = math::dot(plane.normal, c) + plane.distance;
        const math::vec3 an{abs(plane.normal[0]), abs(plane.normal[1]), abs(plane.normal[2])};
        const float r = e[0] * an[0] + e[1] * an[1] + e[2] * an[2];
        return abs(s) <= r;
    }

    bool Intersects(const Aabb& box, const Ray& ray, Result<Aabb, Ray>* result) noexcept
    {
        const float INF = std::numeric_limits<float>::infinity();
        float tmin = 0.0f, tmax = +INF;

        for (std::size_t i = 0; i < 3; ++i)
        {
            const float o = ray.origin[i];
            const float d = ray.direction[i];
            if (d == 0.0f)
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

    bool Intersects(const Aabb& box, const Segment& seg, Result<Aabb, Segment>* result) noexcept
    {
        const math::vec3 dir = Direction(seg); // end-start
        const math::vec3 o = seg.start;

        float tmin = 0.0f, tmax = 1.0f;

        for (std::size_t i = 0; i < 3; ++i)
        {
            const float d = dir[i];
            const float oi = o[i];
            if (d == 0.0f)
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

        // 9 cross-product axes
        const auto axis_test = [&](float a, float b, float fa, float fb, float va0, float va1, float va2, float eb0,
                                   float eb1) -> bool
        {
            float p0 = a * va0 - b * va0; // computed per specific axis below; using inline forms instead
            (void)p0;
            (void)fa;
            (void)fb;
            (void)eb0;
            (void)eb1;
            (void)va1;
            (void)va2;
            return true; // placeholder to satisfy compiler; not used directly
        };

        // (Following the original TAM code layout)
        // Axis L = (1,0,0) x f_i
        {
            float p0 = v0[2] * f0[1] - v0[1] * f0[2];
            float p1 = v1[2] * f0[1] - v1[1] * f0[2];
            float p2 = v2[2] * f0[1] - v2[1] * f0[2];
            float r = e[1] * abs(f0[2]) + e[2] * abs(f0[1]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[2] * f1[1] - v0[1] * f1[2];
            float p1 = v1[2] * f1[1] - v1[1] * f1[2];
            float p2 = v2[2] * f1[1] - v2[1] * f1[2];
            float r = e[1] * abs(f1[2]) + e[2] * abs(f1[1]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[2] * f2[1] - v0[1] * f2[2];
            float p1 = v1[2] * f2[1] - v1[1] * f2[2];
            float p2 = v2[2] * f2[1] - v2[1] * f2[2];
            float r = e[1] * abs(f2[2]) + e[2] * abs(f2[1]);
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
            float r = e[0] * abs(f0[2]) + e[2] * abs(f0[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[0] * f1[2] - v0[2] * f1[0];
            float p1 = v1[0] * f1[2] - v1[2] * f1[0];
            float p2 = v2[0] * f1[2] - v2[2] * f1[0];
            float r = e[0] * abs(f1[2]) + e[2] * abs(f1[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[0] * f2[2] - v0[2] * f2[0];
            float p1 = v1[0] * f2[2] - v1[2] * f2[0];
            float p2 = v2[0] * f2[2] - v2[2] * f2[0];
            float r = e[0] * abs(f2[2]) + e[2] * abs(f2[0]);
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
            float r = e[0] * abs(f0[1]) + e[1] * abs(f0[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[1] * f1[0] - v0[0] * f1[1];
            float p1 = v1[1] * f1[0] - v1[0] * f1[1];
            float p2 = v2[1] * f1[0] - v2[0] * f1[1];
            float r = e[0] * abs(f1[1]) + e[1] * abs(f1[0]);
            if (math::utils::max(math::utils::max(-math::utils::max(p0, math::utils::max(p1, p2)),
                                                  math::utils::min(p0, math::utils::min(p1, p2))),
                                 -r) > r)
                return false;
        }
        {
            float p0 = v0[1] * f2[0] - v0[0] * f2[1];
            float p1 = v1[1] * f2[0] - v1[0] * f2[1];
            float p2 = v2[1] * f2[0] - v2[0] * f2[1];
            float r = e[0] * abs(f2[1]) + e[1] * abs(f2[0]);
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
        const math::vec3 an{abs(n[0]), abs(n[1]), abs(n[2])};
        const float r = e[0] * an[0] + e[1] * an[1] + e[2] * an[2];
        if (d > r || d < -r) return false;

        return true;
    }

    bool Intersects(const Cylinder& a, const Aabb& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Cylinder& a, const Cylinder& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Cylinder& a, const Ellipsoid& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Cylinder& a, const Line& b, Result<Cylinder, Ray>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Cylinder& a, const Obb& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Cylinder& a, const Plane& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Cylinder& a, const Ray& b, Result<Cylinder, Ray>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Cylinder& a, const Segment& b, Result<Cylinder, Ray>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
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

    bool Intersects(const Cylinder& a, const Triangle& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ellipsoid& a, const Aabb& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ellipsoid& a, const Cylinder& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ellipsoid& a, const Ellipsoid& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Line& line, Result<Ellipsoid, Ray>* result) noexcept
    {
        const math::mat3 R = ellipsoid.orientation.to_rotation_matrix();
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

    bool Intersects(const Ellipsoid& a, const Obb& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ellipsoid& a, const Plane& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ellipsoid& ellipsoid, const Ray& ray, Result<Ellipsoid, Ray>* result) noexcept
    {
        const math::mat3 R = ellipsoid.orientation.to_rotation_matrix();
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

    bool Intersects(const Ellipsoid& ellipsoid, const Segment& segment, Result<Ellipsoid, Segment>* result) noexcept
    {
        Result<Ellipsoid, Ray> tmp{};
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

    bool Intersects(const Ellipsoid& a, const Triangle& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Line& a, const Aabb& b, Result<Aabb, Line>* result) noexcept
    {
        return Intersects(b, a, result);
    }

    bool Intersects(const Line& a, const Cylinder& b, Result<Cylinder, Line>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Line& a, const Ellipsoid& b, Result<Ellipsoid, Line>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Line& a, const Line& b, Result<Line, Line>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Line& line, const Obb& obb, Result<Obb, Line>* result) noexcept
    {
        return Intersects(obb, line, result);
    }

    bool Intersects(const Line& line, const Plane& plane, Result<Plane, Line>* result) noexcept
    {
        const float denom = math::dot(plane.normal, line.direction);
        const float num = -(math::dot(plane.normal, line.point) + plane.distance);
        if (denom == 0.0f) return false; // parallel (coincident lines are rare; treat as no unique intersection)
        const float t = num / denom;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Line& a, const Ray& b, Result<Line, Ray>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Line& a, const Segment& b, Result<Line, Segment>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Line& line, const Sphere& sphere, Result<Sphere, Line>* result) noexcept
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

    bool Intersects(const Line& line, const Triangle& triangle, Result<Triangle, Line>* result) noexcept
    {
        return Intersects(triangle, line, result);
    }

    bool Intersects(const Obb& a, const Aabb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Obb& a, const Cylinder& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Obb& a, const Ellipsoid& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Obb& obb, const Line& line, Result<Obb, Line>* result) noexcept
    {
        const math::mat3 R = obb.orientation.to_rotation_matrix();
        const math::mat3 Rt = transpose(R);
        const math::vec3 o = Rt * (line.point - obb.center);
        const math::vec3 d = Rt * line.direction;
        Result<Aabb, Line> tmp{};
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
        const math::mat3 A = a.orientation.to_rotation_matrix(); // localA->world
        const math::mat3 B = b.orientation.to_rotation_matrix(); // localB->world
        const math::mat3 R = transpose(A) * B; // localB in A-space
        math::mat3 AbsR{};
        const float EPS = 1e-6f;
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) AbsR[i][j] = abs(R[i][j]) + EPS;

        const math::vec3 aE = a.half_sizes;
        const math::vec3 bE = b.half_sizes;

        // T in A-space
        const math::vec3 T = transpose(A) * (b.center - a.center);

        // Test A's axes
        for (int i = 0; i < 3; ++i)
        {
            const float ra = aE[i];
            const float rb = bE[0] * AbsR[i][0] + bE[1] * AbsR[i][1] + bE[2] * AbsR[i][2];
            if (abs(T[i]) > ra + rb) return false;
        }

        // Test B's axes
        for (int j = 0; j < 3; ++j)
        {
            const float ra = aE[0] * AbsR[0][j] + aE[1] * AbsR[1][j] + aE[2] * AbsR[2][j];
            const float rb = bE[j];
            const float t = abs(T[0] * R[0][j] + T[1] * R[1][j] + T[2] * R[2][j]);
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
                const float t = abs(T[i2] * R[i1][j] - T[i1] * R[i2][j]);
                if (t > ra + rb) return false;
            }
        return true;
    }

    bool Intersects(const Obb& obb, const Plane& plane) noexcept
    {
        const math::mat3 R = obb.orientation.to_rotation_matrix();
        // projection radius of OBB onto plane normal
        math::vec3 an{abs(plane.normal[0]), abs(plane.normal[1]), abs(plane.normal[2])};
        // Each world axis component projected onto local axes magnitude:
        // r = sum_i e_i * |n · axis_i|
        const math::vec3 e = obb.half_sizes;
        const float r =
            e[0] * abs(R[0][0] * an[0] + R[1][0] * an[1] + R[2][0] * an[2]) +
            e[1] * abs(R[0][1] * an[0] + R[1][1] * an[1] + R[2][1] * an[2]) +
            e[2] * abs(R[0][2] * an[0] + R[1][2] * an[1] + R[2][2] * an[2]);
        const float s = math::dot(plane.normal, obb.center) + plane.distance;
        return abs(s) <= r;
    }

    bool Intersects(const Obb& obb, const Ray& ray, Result<Obb, Ray>* result) noexcept
    {
        const math::mat3 R = obb.orientation.to_rotation_matrix();
        const math::mat3 Rt = transpose(R);
        const math::vec3 o = Rt * (ray.origin - obb.center);
        const math::vec3 d = Rt * ray.direction;

        const Aabb a = MakeAabbFromCenterExtent(math::vec3{0, 0, 0}, obb.half_sizes);
        Result<Aabb, Ray> tmp{};
        const bool hit = Intersects(a, Ray{o, d}, &tmp);
        if (hit && result)
        {
            result->t_min = tmp.t_min;
            result->t_max = tmp.t_max;
        }
        return hit;
    }

    bool Intersects(const Obb& obb, const Segment& segment, Result<Obb, Segment>* result) noexcept
    {
        const math::mat3 R = obb.orientation.to_rotation_matrix();
        const math::mat3 Rt = transpose(R);
        const math::vec3 s = Rt * (segment.start - obb.center);
        const math::vec3 e = Rt * (segment.end - obb.center);
        const Segment ls{s, e};
        Result<Aabb, Segment> tmp{};
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

    bool Intersects(const Obb& a, const Triangle& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Plane& plane, const Aabb& aabb) noexcept
    {
        return Intersects(aabb, plane);
    }

    bool Intersects(const Plane& a, const Cylinder& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Plane& a, const Ellipsoid& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Plane& plane, const Line& line, Result<Plane, Line>* result) noexcept
    {
        return Intersects(line, plane, result);
    }

    bool Intersects(const Plane& a, const Obb& b) noexcept
    {
        return Intersects(b, a);
    }

    bool Intersects(const Plane& a, const Plane& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Plane& plane, const Ray& ray, Result<Plane, Ray>* result) noexcept
    {
        return Intersects(ray, plane, result);
    }

    bool Intersects(const Plane& plane, const Segment& segment, Result<Plane, Segment>* result) noexcept
    {
        return Intersects(segment, plane, result);
    }

    bool Intersects(const Plane& plane, const Sphere& sphere) noexcept
    {
        return abs(math::dot(plane.normal, sphere.center) + plane.distance) <= sphere.radius;
    }

    bool Intersects(const Plane& a, const Triangle& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Ray& ray, const Aabb& box, Result<Aabb, Ray>* result) noexcept
    {
        return Intersects(box, ray, result);
    }

    bool Intersects(const Ray& a, const Cylinder& b, Result<Ray, Cylinder>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Ray& ray, const Ellipsoid& ellipsoid, Result<Ellipsoid, Ray>* result) noexcept
    {
        return Intersects(ellipsoid, ray, result);
    }

    bool Intersects(const Ray& a, const Line& b, Result<Ray, Line>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Ray& ray, const Obb& obb, Result<Obb, Ray>* result) noexcept
    {
        return Intersects(obb, ray, result);
    }

    bool Intersects(const Ray& ray, const Plane& plane, Result<Plane, Ray>* result) noexcept
    {
        const float denom = math::dot(plane.normal, ray.direction);
        const float num = -(math::dot(plane.normal, ray.origin) + plane.distance);
        if (denom == 0.0f) return false;
        const float t = num / denom;
        if (t < 0.0f) return false;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Ray& a, const Ray& b, Result<Ray, Ray>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Ray& a, const Segment& b, Result<Ray, Segment>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Ray& ray, const Sphere& sphere, Result<Sphere, Ray>* result) noexcept
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

    bool Intersects(const Ray& ray, const Triangle& triangle, Result<Triangle, Ray>* result) noexcept
    {
        const float EPS = 1e-8f;
        const math::vec3 e1 = triangle.b - triangle.a;
        const math::vec3 e2 = triangle.c - triangle.a;
        const math::vec3 p = math::cross(ray.direction, e2);
        const float det = math::dot(e1, p);
        if (abs(det) < EPS) return false;
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

    bool Intersects(const Segment& segment, const Aabb& aabb, Result<Aabb, Segment>* result) noexcept
    {
        return Intersects(aabb, segment, result);
    }

    bool Intersects(const Segment& a, const Cylinder& b, Result<Segment, Cylinder>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Segment& segment, const Ellipsoid& ellipsoid, Result<Ellipsoid, Segment>* result) noexcept
    {
        return Intersects(ellipsoid, segment, result);
    }

    bool Intersects(const Segment& a, const Line& b, Result<Segment, Line>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Segment& segment, const Obb& obb, Result<Obb, Segment>* result) noexcept
    {
        return Intersects(obb, segment, result);
    }

    bool Intersects(const Segment& segment, const Plane& plane, Result<Plane, Segment>* result) noexcept
    {
        const math::vec3 d = Direction(segment);
        const float denom = math::dot(plane.normal, d);
        const float num = -(math::dot(plane.normal, segment.start) + plane.distance);
        if (denom == 0.0f) return false;
        const float t = num / denom;
        if (t < 0.0f || t > 1.0f) return false;
        if (result) result->t = t;
        return true;
    }

    bool Intersects(const Segment& a, const Ray& b, Result<Segment, Ray>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Segment& a, const Segment& b, Result<Segment, Segment>* result) noexcept
    {
        //TODO
        if (result)
        {
            //set result values
        }
    }

    bool Intersects(const Segment& segment, const Sphere& sphere, Result<Sphere, Segment>* result) noexcept
    {
        Result<Sphere, Ray> tmp{};
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

    bool Intersects(const Segment& segment, const Triangle& triangle, Result<Triangle, Segment>* result) noexcept
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

    bool Intersects(const Sphere& sphere, const Line& line, Result<Sphere, Line>* result) noexcept
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

    bool Intersects(const Sphere& sphere, const Ray& ray, Result<Sphere, Ray>* result) noexcept
    {
        return Intersects(ray, sphere, result);
    }

    bool Intersects(const Sphere& sphere, const Segment& segment, Result<Sphere, Segment>* result) noexcept
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
        //TODO
    }

    bool Intersects(const Triangle& a, const Ellipsoid& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Triangle& triangle, const Line& line, Result<Triangle, Line>* result) noexcept
    {
        // reuse ray test with both directions; accept any t on line
        const Ray r{line.point, line.direction};
        Result<Triangle, Ray> tmp{};
        if (!Intersects(r, triangle, &tmp)) return false;
        if (result) result->t = tmp.t;
        return true;
    }

    bool Intersects(const Triangle& a, const Obb& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Triangle& a, const Plane& b) noexcept
    {
        //TODO
    }

    bool Intersects(const Triangle& triangle, const Ray& ray, Result<Triangle, Ray>* result) noexcept
    {
        return Intersects(ray, triangle, result);
    }

    bool Intersects(const Triangle& triangle, const Segment& segment, Result<Triangle, Segment>* result) noexcept
    {
        Result<Triangle, Ray> tmp{};
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
        //TODO
    }

    //------------------------------------------------------------------------------------------------------------------

    bool Contains(const Aabb& outer, const math::vec3& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Aabb& outer, const Aabb& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
    }

    bool Contains(const Aabb& outer, const Cylinder& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Aabb& outer, const Ellipsoid& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Aabb& outer, const Obb& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
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

    bool Contains(const Aabb& outer, const Triangle& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Contains(const Cylinder& outer, const math::vec3& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Cylinder& outer, const Aabb& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
    }

    bool Contains(const Cylinder& outer, const Cylinder& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Cylinder& outer, const Ellipsoid& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Cylinder& outer, const Obb& inner) noexcept
    {
        //TODO
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

        if (axis_len_sq == 0.0f)
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
        const math::mat3 R = ellipsoid.orientation.to_rotation_matrix();
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
        //TODO is there a faster algorithm than testing each vertex?
    }

    bool Contains(const Ellipsoid& outer, const Cylinder& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Ellipsoid& outer, const Ellipsoid& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Ellipsoid& outer, const Obb& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
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
            return math::length_squared(unit) == 0.0f;
        }

        const math::mat3 R = outer.orientation.to_rotation_matrix();
        const math::mat3 Rt = transpose(R);
        const math::vec3 local = Rt * (inner.center - outer.center);

        const math::vec3 scaled{local[0] / radii[0], local[1] / radii[1], local[2] / radii[2]};
        const float scaled_len = math::length(scaled);
        if (scaled_len >= 1.0f) return false; // center is outside or on boundary

        if (math::length_squared(local) == 0.0f)
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
        //TODO
    }

    bool Contains(const Obb& outer, const Aabb& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
    }

    bool Contains(const Obb& outer, const Cylinder& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Obb& outer, const Ellipsoid& inner) noexcept
    {
        //TODO
    }

    bool Contains(const Obb& outer, const Obb& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
    }

    bool Contains(const Obb& outer, const Segment& inner) noexcept
    {
        //TODO is there a faster algorithm than testing each vertex?
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Obb& outer, const Sphere& inner) noexcept
    {
        const math::mat3 R = outer.orientation.to_rotation_matrix();
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
            farthest[i] = (abs(diff_min[i]) > abs(diff_max[i])) ? inner.min[i] : inner.max[i];
        }
        const math::vec3 diff = farthest - outer.center;
        return math::dot(diff, diff) <= outer.radius * outer.radius;
    }

    bool Contains(const Sphere& outer, const Cylinder& inner) noexcept
    {
        const math::vec3 axis_dir = AxisDirection(inner);
        const float axis_len_sq = math::length_squared(axis_dir);
        const math::vec3 delta = inner.center - outer.center;

        if (axis_len_sq == 0.0f)
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
        const math::mat3 R = inner.orientation.to_rotation_matrix();
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

        auto evaluate = [&](double lambda) noexcept {
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
        if (diff_len_sq == 0.0f)
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
} // namespace engine::geometry
