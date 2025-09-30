#include "engine/geometry/shapes/sphere.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/cylinder.hpp"

namespace engine::geometry {
namespace {

[[nodiscard]] std::array<math::vec3, 8> corners(const Aabb& box) noexcept {
    std::array<math::vec3, 8> result{};
    std::size_t index = 0U;

    for (int x = 0; x <= 1; ++x) {
        for (int y = 0; y <= 1; ++y) {
            for (int z = 0; z <= 1; ++z) {
                result[index++] = math::vec3{
                    x ? box.max[0] : box.min[0],
                    y ? box.max[1] : box.min[1],
                    z ? box.max[2] : box.min[2],
                };
            }
        }
    }

    return result;
}

[[nodiscard]] std::array<math::vec3, 8> corners(const Obb& box) noexcept {
    std::array<math::vec3, 8> result{};
    std::size_t index = 0U;

    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            for (int z = -1; z <= 1; z += 2) {
                const math::vec3 local_corner{
                    static_cast<float>(x) * box.half_sizes[0],
                    static_cast<float>(y) * box.half_sizes[1],
                    static_cast<float>(z) * box.half_sizes[2],
                };
                result[index++] = box.center + box.orientation * local_corner;
            }
        }
    }

    return result;
}

}  // namespace

float surface_area(const Sphere& s) noexcept {
    return static_cast<float>(4.0) * std::numbers::pi_v<float> * s.radius * s.radius;
}

float volume(const Sphere& s) noexcept {
    return static_cast<float>(4.0 / 3.0) * std::numbers::pi_v<float> * s.radius * s.radius * s.radius;
}

bool contains(const Sphere& s, const math::vec3& point) noexcept {
    const math::vec3 offset = point - s.center;
    return math::length_squared(offset) <= s.radius * s.radius;
}

bool contains(const Sphere& outer, const Sphere& inner) noexcept {
    const math::vec3 offset = inner.center - outer.center;
    const float distance = math::length(offset);
    return distance + inner.radius <= outer.radius + std::numeric_limits<float>::epsilon();
}

bool contains(const Sphere& outer, const Aabb& inner) noexcept {
    for (const auto& corner : corners(inner)) {
        if (!contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool contains(const Sphere& outer, const Obb& inner) noexcept {
    for (const auto& corner : corners(inner)) {
        if (!contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool intersects(const Sphere& lhs, const Sphere& rhs) noexcept {
    const math::vec3 offset = rhs.center - lhs.center;
    const float radii = lhs.radius + rhs.radius;
    return math::length_squared(offset) <= radii * radii;
}

bool intersects(const Sphere& s, const Aabb& box) noexcept {
    return intersects(box, s);
}

bool intersects(const Sphere& s, const Obb& box) noexcept {
    return intersects(box, s);
}

bool intersects(const Sphere& s, const Cylinder& c) noexcept {
    return intersects(c, s);
}

Sphere make_sphere_from_point(const math::vec3& point) noexcept {
    return Sphere{point, 0.0f};
}

Sphere bounding_sphere(const Aabb& box) noexcept {
    const math::vec3 c = center(box);
    const math::vec3 ext = extent(box);
    return Sphere{c, math::length(ext)};
}

Sphere bounding_sphere(const Obb& box) noexcept {
    float max_distance_sq = 0.0f;
    for (const auto& corner : corners(box)) {
        const float dist_sq = math::length_squared(corner - box.center);
        max_distance_sq = std::max(max_distance_sq, dist_sq);
    }
    return Sphere{box.center, std::sqrt(max_distance_sq)};
}

}  // namespace engine::geometry

