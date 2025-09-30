#include "engine/geometry/shapes/ray.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/geometry/shapes/sphere.hpp"

namespace engine::geometry {

math::vec3 point_at(const Ray& r, float t) noexcept {
    return r.origin + r.direction * t;
}

bool intersects(const Ray& r, const Aabb& box, float& out_t_min, float& out_t_max) noexcept {
    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::infinity();

    for (std::size_t i = 0; i < 3; ++i) {
        const float dir = r.direction[i];
        const float origin = r.origin[i];
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

        t_min = std::max(t_min, t0);
        t_max = std::min(t_max, t1);
        if (t_max < t_min) {
            return false;
        }
    }

    out_t_min = t_min;
    out_t_max = t_max;
    return true;
}

bool intersects(const Ray& r, const Sphere& s, float& out_t) noexcept {
    const math::vec3 m = r.origin - s.center;
    const float b = math::dot(m, r.direction);
    const float c = math::dot(m, m) - s.radius * s.radius;

    if (c > 0.0f && b > 0.0f) {
        return false;
    }

    const float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        return false;
    }

    float t = -b - std::sqrt(discriminant);
    if (t < 0.0f) {
        t = -b + std::sqrt(discriminant);
    }

    if (t < 0.0f) {
        return false;
    }

    out_t = t;
    return true;
}

bool intersects(const Ray& r, const Plane& p, float& out_t) noexcept {
    return intersects(p, r, out_t);
}

}  // namespace engine::geometry

