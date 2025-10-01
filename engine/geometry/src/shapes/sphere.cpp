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

[[nodiscard]] std::array<math::vec3, 8> Corners(const Obb& box) noexcept {
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

bool Contains(const Sphere& s, const math::vec3& point) noexcept {
    const math::vec3 offset = point - s.center;
    return math::length_squared(offset) <= s.radius * s.radius;
}

bool Contains(const Sphere& outer, const Sphere& inner) noexcept {
    const math::vec3 offset = inner.center - outer.center;
    const float distance = math::length(offset);
    return distance + inner.radius <= outer.radius + std::numeric_limits<float>::epsilon();
}

bool Contains(const Sphere& outer, const Aabb& inner) noexcept {
    for (const auto& corner : Corners(inner)) {
        if (!Contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool Contains(const Sphere& outer, const Obb& inner) noexcept {
    for (const auto& corner : Corners(inner)) {
        if (!Contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool Intersects(const Sphere& lhs, const Sphere& rhs) noexcept {
    const math::vec3 offset = rhs.center - lhs.center;
    const float radii = lhs.radius + rhs.radius;
    return math::length_squared(offset) <= radii * radii;
}

bool Intersects(const Sphere& s, const Aabb& box) noexcept {
    return Intersects(box, s);
}

bool Intersects(const Sphere& s, const Obb& box) noexcept {
    return Intersects(box, s);
}

bool Intersects(const Sphere& s, const Cylinder& c) noexcept {
    return Intersects(c, s);
}

Sphere make_sphere_from_point(const math::vec3& point) noexcept {
    return Sphere{point, 0.0f};
}

Sphere bounding_sphere(const Aabb& box) noexcept {
    const math::vec3 c = Center(box);
    const math::vec3 ext = Extent(box);
    return Sphere{c, math::length(ext)};
}

Sphere bounding_sphere(const Obb& box) noexcept {
    float max_distance_sq = 0.0f;
    for (const auto& corner : Corners(box)) {
        const float dist_sq = math::length_squared(corner - box.center);
        max_distance_sq = std::max(max_distance_sq, dist_sq);
    }
    return Sphere{box.center, std::sqrt(max_distance_sq)};
}

}  // namespace engine::geometry

