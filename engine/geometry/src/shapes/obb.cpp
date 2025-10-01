#include "engine/geometry/shapes/obb.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/math/matrix.hpp"

namespace engine::geometry {
namespace {

[[nodiscard]] constexpr float two() noexcept { return 2.0f; }
[[nodiscard]] constexpr float epsilon() noexcept { return 1e-6f; }

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

math::vec3 size(const Obb& box) noexcept {
    return box.half_sizes * two();
}

math::vec3 extent(const Obb& box) noexcept {
    return box.half_sizes;
}

bool Contains(const Obb& box, const math::vec3& point) noexcept {
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

bool Contains(const Obb& outer, const Obb& inner) noexcept {
    for (const auto& corner : Corners(inner)) {
        if (!Contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool Contains(const Obb& outer, const Aabb& inner) noexcept {
    for (const auto& corner : Corners(inner)) {
        if (!Contains(outer, corner)) {
            return false;
        }
    }
    return true;
}

bool Contains(const Obb& outer, const Sphere& inner) noexcept {
    const math::mat3 inverse_orientation = math::transpose(outer.orientation);
    const math::vec3 local = inverse_orientation * (inner.center - outer.center);

    for (std::size_t i = 0; i < 3; ++i) {
        if (std::fabs(local[i]) + inner.radius > outer.half_sizes[i]) {
            return false;
        }
    }
    return true;
}

bool Intersects(const Obb& lhs, const Obb& rhs) noexcept {
    const math::mat3 R = math::transpose(lhs.orientation) * rhs.orientation;
    math::mat3 AbsR{};
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            AbsR[i][j] = std::fabs(R[i][j]) + epsilon();
        }
    }

    const math::vec3 t_world = rhs.center - lhs.center;
    const math::vec3 t = math::transpose(lhs.orientation) * t_world;

    for (std::size_t i = 0; i < 3; ++i) {
        const float ra = lhs.half_sizes[i];
        float rb = 0.0f;
        for (std::size_t j = 0; j < 3; ++j) {
            rb += rhs.half_sizes[j] * AbsR[i][j];
        }
        if (std::fabs(t[i]) > ra + rb) {
            return false;
        }
    }

    for (std::size_t j = 0; j < 3; ++j) {
        float ra = 0.0f;
        for (std::size_t i = 0; i < 3; ++i) {
            ra += lhs.half_sizes[i] * AbsR[i][j];
        }
        const float rb = rhs.half_sizes[j];
        float t_proj = 0.0f;
        for (std::size_t i = 0; i < 3; ++i) {
            t_proj += t[i] * R[i][j];
        }
        if (std::fabs(t_proj) > ra + rb) {
            return false;
        }
    }

    for (std::size_t i = 0; i < 3; ++i) {
        const std::size_t i1 = (i + 1) % 3;
        const std::size_t i2 = (i + 2) % 3;
        for (std::size_t j = 0; j < 3; ++j) {
            const std::size_t j1 = (j + 1) % 3;
            const std::size_t j2 = (j + 2) % 3;

            const float ra = lhs.half_sizes[i1] * AbsR[i2][j] + lhs.half_sizes[i2] * AbsR[i1][j];
            const float rb = rhs.half_sizes[j1] * AbsR[i][j2] + rhs.half_sizes[j2] * AbsR[i][j1];
            const float tval = std::fabs(t[i2] * R[i1][j] - t[i1] * R[i2][j]);
            if (tval > ra + rb) {
                return false;
            }
        }
    }

    return true;
}

bool Intersects(const Obb& box, const Aabb& other) noexcept {
    return Intersects(box, from_aabb(other));
}

bool Intersects(const Obb& box, const Sphere& s) noexcept {
    const math::mat3 inverse_orientation = math::transpose(box.orientation);
    const math::vec3 local = inverse_orientation * (s.center - box.center);

    math::vec3 clamped{};
    for (std::size_t i = 0; i < 3; ++i) {
        clamped[i] = std::clamp(local[i], -box.half_sizes[i], box.half_sizes[i]);
    }

    const math::vec3 diff = local - clamped;
    return math::length_squared(diff) <= s.radius * s.radius;
}

Obb from_aabb(const Aabb& box) noexcept {
    const math::vec3 ext = Extent(box);
    return Obb{Center(box), ext, math::identity_matrix<float, 3>()};
}

Obb from_center_half_sizes(const math::vec3& center, const math::vec3& half_sizes) noexcept {
    return Obb{center, half_sizes, math::identity_matrix<float, 3>()};
}

}  // namespace engine::geometry

