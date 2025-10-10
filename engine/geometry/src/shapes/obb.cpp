#include "engine/geometry/shapes.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/utils.hpp"
#include "engine/math/utils_rotation.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace engine::geometry {
    math::vec3 Size(const Obb &box) noexcept {
        return box.half_sizes * 2.0f;
    }

    math::vec3 Extent(const Obb &box) noexcept {
        return box.half_sizes;
    }

    double SurfaceArea(const Obb &box) noexcept {
        return (box.half_sizes[0] * box.half_sizes[1] +
                box.half_sizes[1] * box.half_sizes[2] +
                box.half_sizes[0] * box.half_sizes[2]) * 8.0;
    }

    double Volume(const Obb &box) noexcept {
        return box.half_sizes[0] * box.half_sizes[1] * box.half_sizes[2] * 8.0;
    }

    Obb MakeObbFromCenterHalfSizes(const math::vec3 &center, const math::vec3 &half_sizes,
                                   const math::quat &orientation) noexcept {
        return Obb{center, half_sizes, orientation};
    }

    Obb BoundingObb(const Aabb &box) noexcept {
        const math::vec3 ext = Extent(box);
        return Obb{Center(box), ext, {}};
    }

    Obb BoundingObb(const Sphere &s) noexcept {
        const math::vec3 ext = math::vec3{s.radius};
        return Obb{s.center, ext, {}};
    }

    Obb BoundingObb(const Obb &box, const math::mat4 &transform) noexcept {
        const std::array<math::vec3, 8> corners = GetCorners(box);
        std::array<math::vec3, 8> transformed_corners{};
        for (std::size_t i = 0; i < corners.size(); ++i) {
            const math::vec4 corner4{corners[i][0], corners[i][1], corners[i][2], 1.0f};
            const math::vec4 transformed4 = transform * corner4;
            transformed_corners[i] = math::vec3{transformed4[0], transformed4[1], transformed4[2]};
        }

        const math::vec4 center4{box.center[0], box.center[1], box.center[2], 1.0f};
        const math::vec4 transformed_center4 = transform * center4;
        math::vec3 new_center{transformed_center4[0], transformed_center4[1], transformed_center4[2]};

        math::mat3 linear{};
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 3; ++c) {
                linear[r][c] = transform[r][c];
            }
        }

        const math::mat3 rotation = math::utils::to_rotation_matrix<float>(box.orientation);
        std::array<math::vec3, 3> axes{};
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const math::vec3 basis{rotation[0][axis], rotation[1][axis], rotation[2][axis]};
            axes[axis] = linear * basis;
        }

        auto normalize_safe = [](const math::vec3 &v, const math::vec3 &fallback) noexcept {
            const float len_sq = math::length_squared(v);
            if (len_sq == 0.0f) {
                return fallback;
            }
            const float inv_len = 1.0f / math::utils::sqrt(len_sq);
            return v * inv_len;
        };

        math::vec3 axis0 = normalize_safe(axes[0], math::vec3{1.0f, 0.0f, 0.0f});
        math::vec3 axis1_candidate = axes[1] - axis0 * math::dot(axis0, axes[1]);
        math::vec3 axis1 = normalize_safe(axis1_candidate, math::vec3{0.0f, 1.0f, 0.0f});
        if (math::length_squared(axis1) == 0.0f) {
            const math::vec3 fallback = std::fabs(axis0[0]) > 0.5f ? math::vec3{0.0f, 1.0f, 0.0f} : math::vec3{1.0f, 0.0f, 0.0f};
            axis1 = normalize_safe(fallback - axis0 * math::dot(axis0, fallback), math::vec3{0.0f, 1.0f, 0.0f});
        }
        math::vec3 axis2 = math::cross(axis0, axis1);
        axis2 = normalize_safe(axis2, math::vec3{0.0f, 0.0f, 1.0f});

        math::mat3 orientation_matrix{};
        for (std::size_t row = 0; row < 3; ++row) {
            orientation_matrix[row][0] = axis0[row];
            orientation_matrix[row][1] = axis1[row];
            orientation_matrix[row][2] = axis2[row];
        }

        math::vec3 new_half_sizes{0.0f};
        for (std::size_t axis = 0; axis < 3; ++axis) {
            float min_proj = std::numeric_limits<float>::max();
            float max_proj = std::numeric_limits<float>::lowest();
            const math::vec3 axis_dir{orientation_matrix[0][axis], orientation_matrix[1][axis], orientation_matrix[2][axis]};
            for (const auto &corner: transformed_corners) {
                const math::vec3 offset = corner - new_center;
                const float projection = math::dot(offset, axis_dir);
                min_proj = math::utils::min(min_proj, projection);
                max_proj = math::utils::max(max_proj, projection);
            }
            new_half_sizes[axis] = 0.5f * (max_proj - min_proj);
        }

        const math::quat new_orientation = math::from_rotation_matrix(orientation_matrix);
        return Obb{new_center, new_half_sizes, new_orientation};
    }

    Obb BoundingObb(std::span<math::vec3> points) noexcept {
        if (points.empty()) {
            return Obb{math::vec3{0.0f}, math::vec3{0.0f}, {}};
        }

        const Aabb bounds = BoundingAabb(points);
        return BoundingObb(bounds);
    }

    Obb FromCenterHalfSizes(const math::vec3 &center, const math::vec3 &half_sizes) noexcept {
        return Obb{center, half_sizes, {}};
    }

    math::vec3 ClosestPoint(const Obb &obb, const math::vec3 &point) noexcept {
        const math::mat3 rotation = math::utils::to_rotation_matrix(obb.orientation);
        const math::mat3 rotation_transposed = math::transpose(rotation);
        const math::vec3 local_point = rotation_transposed * (point - obb.center);

        const math::vec3 half_sizes = obb.half_sizes;
        const math::vec3 clamped{
            math::utils::clamp(local_point[0], -half_sizes[0], half_sizes[0]),
            math::utils::clamp(local_point[1], -half_sizes[1], half_sizes[1]),
            math::utils::clamp(local_point[2], -half_sizes[2], half_sizes[2])
        };

        return obb.center + rotation * clamped;
    }

    double SquaredDistance(const Obb &box, const math::vec3 &point) noexcept {
        const math::vec3 closest = ClosestPoint(box, point);
        const math::vec3 diff = point - closest;
        return static_cast<double>(math::dot(diff, diff));
    }

    std::array<math::vec3, 8> GetCorners(const Obb &box) noexcept {
        std::array<math::vec3, 8> vertices{};
        math::mat3 R = math::utils::to_rotation_matrix(box.orientation);
        math::vec3 he = box.half_sizes;

        // Calculate the 8 vertices of the OBB
        vertices[0] = box.center + R * math::vec3(-he[0], -he[1], -he[2]);
        vertices[1] = box.center + R * math::vec3(he[0], -he[1], -he[2]);
        vertices[2] = box.center + R * math::vec3(he[0], he[1], -he[2]);
        vertices[3] = box.center + R * math::vec3(-he[0], he[1], -he[2]);
        vertices[4] = box.center + R * math::vec3(-he[0], -he[1], he[2]);
        vertices[5] = box.center + R * math::vec3(he[0], -he[1], he[2]);
        vertices[6] = box.center + R * math::vec3(he[0], he[1], he[2]);
        vertices[7] = box.center + R * math::vec3(-he[0], he[1], he[2]);

        return vertices;
    }
} // namespace engine::geometry
