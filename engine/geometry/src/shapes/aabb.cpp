#include "engine/geometry/shapes/aabb.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/sphere.hpp"

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float half() noexcept { return 0.5f; }
[[nodiscard]] constexpr float two() noexcept { return 2.0f; }

[[nodiscard]] std::array<math::vec3, 8> corners(const obb& box) noexcept {
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

bool contains(const aabb& outer, const aabb& inner) noexcept {
    for (std::size_t i = 0; i < 3; ++i) {
        if (inner.min[i] < outer.min[i] || inner.max[i] > outer.max[i]) {
            return false;
        }
    }
    return true;
}

bool contains(const aabb& outer, const sphere& inner) noexcept {
    for (std::size_t i = 0; i < 3; ++i) {
        if (inner.center[i] - inner.radius < outer.min[i]) {
            return false;
        }
        if (inner.center[i] + inner.radius > outer.max[i]) {
            return false;
        }
    }
    return true;
}

bool contains(const aabb& outer, const obb& inner) noexcept {
    for (const auto& corner : corners(inner)) {
        if (!contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool intersects(const aabb& lhs, const aabb& rhs) noexcept {
    for (std::size_t i = 0; i < 3; ++i) {
        if (lhs.max[i] < rhs.min[i] || lhs.min[i] > rhs.max[i]) {
            return false;
        }
    }
    return true;
}

bool intersects(const aabb& box, const sphere& s) noexcept {
    math::vec3 closest{};
    for (std::size_t i = 0; i < 3; ++i) {
        closest[i] = std::clamp(s.center[i], box.min[i], box.max[i]);
    }
    const math::vec3 offset = closest - s.center;
    return math::length_squared(offset) <= s.radius * s.radius;
}

bool intersects(const aabb& box, const obb& other) noexcept {
    return intersects(other, box);
}

aabb make_aabb_from_point(const math::vec3& point) noexcept {
    return aabb{point, point};
}

aabb make_aabb_from_center_extent(const math::vec3& center, const math::vec3& ext) noexcept {
    return aabb{center - ext, center + ext};
}

aabb bounding_aabb(const sphere& s) noexcept {
    const math::vec3 radius_vec{s.radius};
    return aabb{s.center - radius_vec, s.center + radius_vec};
}

}  // namespace engine::geometry

