#include "engine/geometry/shapes/cylinder.hpp"

#include <cmath>
#include <numbers>

#include "engine/geometry/shapes/sphere.hpp"

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float two() noexcept { return 2.0f; }

}  // namespace

math::vec3 axis_direction(const Cylinder& c) noexcept {
    const float len = math::length(c.axis);
    if (len == 0.0f) {
        return math::vec3{0.0f};
    }
    return c.axis / len;
}

math::vec3 top_center(const Cylinder& c) noexcept {
    return c.center + axis_direction(c) * c.half_height;
}

math::vec3 bottom_center(const Cylinder& c) noexcept {
    return c.center - axis_direction(c) * c.half_height;
}

float volume(const Cylinder& c) noexcept {
    const float height = c.half_height * two();
    return std::numbers::pi_v<float> * c.radius * c.radius * height;
}

float lateral_surface_area(const Cylinder& c) noexcept {
    const float height = c.half_height * two();
    return two() * std::numbers::pi_v<float> * c.radius * height;
}

float surface_area(const Cylinder& c) noexcept {
    return lateral_surface_area(c) + two() * std::numbers::pi_v<float> * c.radius * c.radius;
}

bool contains(const Cylinder& c, const math::vec3& point) noexcept {
    const math::vec3 axis_dir = axis_direction(c);
    if (math::length_squared(axis_dir) == 0.0f) {
        return false;
    }

    const math::vec3 relative = point - c.center;
    const float height = math::dot(relative, axis_dir);
    if (height < -c.half_height || height > c.half_height) {
        return false;
    }

    const math::vec3 radial = relative - axis_dir * height;
    return math::length_squared(radial) <= c.radius * c.radius;
}

bool intersects(const Cylinder& c, const Sphere& s) noexcept {
    const math::vec3 axis_dir = axis_direction(c);
    if (math::length_squared(axis_dir) == 0.0f) {
        return false;
    }

    const math::vec3 delta = s.center - c.center;
    const float height = math::dot(delta, axis_dir);
    const math::vec3 radial_vec = delta - axis_dir * height;
    const float radial_len = math::length(radial_vec);
    float distance_sq = 0.0f;

    if (height < -c.half_height) {
        const float excess_height = -c.half_height - height;
        if (radial_len <= c.radius) {
            distance_sq = excess_height * excess_height;
        } else {
            const float radial_excess = radial_len - c.radius;
            distance_sq = excess_height * excess_height + radial_excess * radial_excess;
        }
    } else if (height > c.half_height) {
        const float excess_height = height - c.half_height;
        if (radial_len <= c.radius) {
            distance_sq = excess_height * excess_height;
        } else {
            const float radial_excess = radial_len - c.radius;
            distance_sq = excess_height * excess_height + radial_excess * radial_excess;
        }
    } else {
        if (radial_len <= c.radius) {
            distance_sq = 0.0f;
        } else {
            const float radial_excess = radial_len - c.radius;
            distance_sq = radial_excess * radial_excess;
        }
    }

    return distance_sq <= s.radius * s.radius;
}

}  // namespace engine::geometry

