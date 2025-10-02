#include "engine/geometry/shapes/ray.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/geometry/shapes.hpp"

namespace engine::geometry {
    math::vec3 PointAt(const Ray &r, float t) noexcept {
        return r.origin + r.direction * t;
    }

    bool Intersects(const Ray &ray,
                    const Aabb &box,
                    RayHit *result) noexcept {
        float t_in = 0.0f;
        float t_out = std::numeric_limits<float>::infinity();

        for (std::size_t i = 0; i < 3; ++i) {
            const float dir = ray.direction[i];
            const float origin = ray.origin[i];
            if (std::fabs(dir) < std::numeric_limits<float>::epsilon()) {
                if (origin < box.min[i] || origin > box.max[i]) {
                    return false;
                }
                continue;
            }

            const float inv_dir = 1.0f / dir;
            float t0 = (box.min[i] - origin) * inv_dir;
            float t1 = (box.max[i] - origin) * inv_dir;
            if (t0 > t1) {
                std::swap(t0, t1);
            }

            t_in = std::max(t_in, t0);
            t_out = std::min(t_out, t1);
            if (t_out < t_in) {
                return false;
            }
        }
        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Cylinder &cylinder,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray, const
                    Ellipsoid &ellipsoid,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Line &line,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Obb &obb,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Plane &plane,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Ray &other,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Sphere &sphere,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }

    bool Intersects(const Ray &ray,
                    const Triangle &triangle,
                    RayHit *result) noexcept {
        float t_in = -1, t_out = -1;
        //TODO

        if (result) {
            result->t_in = t_in;
            result->t_out = t_out;
        }
        return true;
    }
} // namespace engine::geometry
