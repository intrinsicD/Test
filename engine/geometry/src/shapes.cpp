#include "engine/geometry/shapes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float half() noexcept { return 0.5f; }

[[nodiscard]] constexpr float two() noexcept { return 2.0f; }

}  // namespace

math::vec3 center(const aabb& box) noexcept {
    return (box.min + box.max) * half();
}

math::vec3 size(const aabb& box) noexcept {
    return box.max - box.min;
}

math::vec3 extent(const aabb& box) noexcept {
    return size(box) * half();
}

float surface_area(const aabb& box) noexcept {
    const math::vec3 s = size(box);
    return two() * (s[0] * s[1] + s[1] * s[2] + s[0] * s[2]);
}

float volume(const aabb& box) noexcept {
    const math::vec3 s = size(box);
    return s[0] * s[1] * s[2];
}

bool contains(const aabb& box, const math::vec3& point) noexcept {
    for (std::size_t i = 0; i < 3; ++i) {
        if (point[i] < box.min[i] || point[i] > box.max[i]) {
            return false;
        }
    }
    return true;
}

math::vec3 size(const obb& box) noexcept {
    return box.half_sizes * two();
}

math::vec3 extent(const obb& box) noexcept {
    return box.half_sizes;
}

bool contains(const obb& box, const math::vec3& point) noexcept {
    const math::vec3 relative = point - box.center;
    const math::mat3 inverse_orientation = math::transpose(box.orientation);
    const math::vec3 local = inverse_orientation * relative;

    for (std::size_t i = 0; i < 3; ++i) {
        if (std::fabs(local[i]) > box.half_sizes[i]) {
            return false;
        }
    }
    return true;
}

aabb bounding_aabb(const obb& box) noexcept {
    std::array<math::vec3, 8> corners{};
    std::size_t index = 0U;

    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            for (int z = -1; z <= 1; z += 2) {
                const math::vec3 local_corner{
                    static_cast<float>(x) * box.half_sizes[0],
                    static_cast<float>(y) * box.half_sizes[1],
                    static_cast<float>(z) * box.half_sizes[2],
                };
                corners[index++] = box.center + box.orientation * local_corner;
            }
        }
    }

    math::vec3 min_corner{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };

    math::vec3 max_corner{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (const auto& corner : corners) {
        for (std::size_t i = 0; i < 3; ++i) {
            min_corner[i] = std::min(min_corner[i], corner[i]);
            max_corner[i] = std::max(max_corner[i], corner[i]);
        }
    }

    return aabb{min_corner, max_corner};
}

float surface_area(const sphere& s) noexcept {
    return static_cast<float>(4.0) * std::numbers::pi_v<float> * s.radius * s.radius;
}

float volume(const sphere& s) noexcept {
    return static_cast<float>(4.0 / 3.0) * std::numbers::pi_v<float> * s.radius * s.radius * s.radius;
}

bool contains(const sphere& s, const math::vec3& point) noexcept {
    const math::vec3 offset = point - s.center;
    return math::length_squared(offset) <= s.radius * s.radius;
}

float signed_distance(const plane& p, const math::vec3& point) noexcept {
    return math::dot(p.normal, point) + p.distance;
}

math::vec3 project_point(const plane& p, const math::vec3& point) noexcept {
    const float denom = math::length_squared(p.normal);
    if (denom == 0.0f) {
        return point;
    }
    const float dist = signed_distance(p, point);
    return point - p.normal * (dist / denom);
}

bool contains(const plane& p, const math::vec3& point, float epsilon) noexcept {
    return std::fabs(signed_distance(p, point)) <= epsilon;
}

math::vec3 point_at(const ray& r, float t) noexcept {
    return r.origin + r.direction * t;
}

math::vec3 direction(const segment& s) noexcept {
    return s.end - s.start;
}

float length(const segment& s) noexcept {
    return math::length(direction(s));
}

math::vec3 point_at(const segment& s, float t) noexcept {
    return s.start + direction(s) * t;
}

math::vec3 point_at(const line& l, float t) noexcept {
    return l.point + l.direction * t;
}

math::vec3 project_point(const line& l, const math::vec3& point) noexcept {
    const float denom = math::length_squared(l.direction);
    if (denom == 0.0f) {
        return l.point;
    }
    const math::vec3 offset = point - l.point;
    const float t = math::dot(offset, l.direction) / denom;
    return point_at(l, t);
}

float volume(const ellipsoid& e) noexcept {
    return static_cast<float>(4.0 / 3.0) * std::numbers::pi_v<float> * e.radii[0] * e.radii[1] * e.radii[2];
}

bool contains(const ellipsoid& e, const math::vec3& point) noexcept {
    const math::vec3 relative = point - e.center;
    const math::mat3 inverse_orientation = math::transpose(e.orientation);
    const math::vec3 local = inverse_orientation * relative;

    float sum = 0.0f;
    for (std::size_t i = 0; i < 3; ++i) {
        if (e.radii[i] == 0.0f) {
            if (local[i] != 0.0f) {
                return false;
            }
            continue;
        }
        const float scaled = local[i] / e.radii[i];
        sum += scaled * scaled;
    }

    return sum <= 1.0f + std::numeric_limits<float>::epsilon();
}

math::vec3 normal(const triangle& t) noexcept {
    return math::cross(t.b - t.a, t.c - t.a);
}

math::vec3 unit_normal(const triangle& t) noexcept {
    return math::normalize(normal(t));
}

float area(const triangle& t) noexcept {
    return half() * math::length(normal(t));
}

math::vec3 centroid(const triangle& t) noexcept {
    return (t.a + t.b + t.c) / static_cast<float>(3.0);
}

math::vec3 axis_direction(const cylinder& c) noexcept {
    const float len = math::length(c.axis);
    if (len == 0.0f) {
        return math::vec3{0.0f};
    }
    return c.axis / len;
}

math::vec3 top_center(const cylinder& c) noexcept {
    return c.center + axis_direction(c) * c.half_height;
}

math::vec3 bottom_center(const cylinder& c) noexcept {
    return c.center - axis_direction(c) * c.half_height;
}

float volume(const cylinder& c) noexcept {
    const float height = c.half_height * two();
    return std::numbers::pi_v<float> * c.radius * c.radius * height;
}

float lateral_surface_area(const cylinder& c) noexcept {
    const float height = c.half_height * two();
    return two() * std::numbers::pi_v<float> * c.radius * height;
}

float surface_area(const cylinder& c) noexcept {
    return lateral_surface_area(c) + two() * std::numbers::pi_v<float> * c.radius * c.radius;
}

bool contains(const cylinder& c, const math::vec3& point) noexcept {
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

}  // namespace engine::geometry

