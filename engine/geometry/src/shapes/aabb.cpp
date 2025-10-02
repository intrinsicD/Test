#include "engine/geometry/shapes/aabb.hpp"

#include "engine/geometry/shapes.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace engine::geometry {
    math::vec3 Center(const Aabb &box) noexcept {
        return (box.min + box.max) * 0.5f;
    }

    math::vec3 Size(const Aabb &box) noexcept {
        return box.max - box.min;
    }

    math::vec3 Extent(const Aabb &box) noexcept {
        return Size(box) * 0.5f;
    }

    double SurfaceArea(const Aabb &box) noexcept {
        const math::vec3 s = Size(box);
        return 2.0 * (s[0] * s[1] + s[1] * s[2] + s[0] * s[2]);
    }

    double Volume(const Aabb &box) noexcept {
        const math::vec3 s = Size(box);
        return s[0] * s[1] * s[2];
    }


    Aabb BoundingAabb(const math::vec3 &point) noexcept {
        return Aabb{point, point};
    }

    Aabb BoundingAabb(std::span<math::vec3> points) noexcept {
        Aabb result;
        if (points.empty()) {
            result.min = math::vec3{0.0f};
            result.max = math::vec3{0.0f};
            return result;
        }
        result.min = math::vec3{
            std::numeric_limits<float>::max()
        };
        result.max = math::vec3{
            std::numeric_limits<float>::lowest()
        };
        for (const auto &point: points) {
            Merge(result, point);
        }
        return result;
    }

    Aabb BoundingAabb(std::span<Aabb> aabbs) noexcept {
        Aabb result;
        if (aabbs.empty()) {
            result.min = math::vec3{0.0f};
            result.max = math::vec3{0.0f};
            return result;
        }
        result.min = math::vec3{
            std::numeric_limits<float>::max()
        };
        result.max = math::vec3{
            std::numeric_limits<float>::lowest()
        };
        for (const auto &aabb: aabbs) {
            Merge(result, aabb);
        }
        return result;
    }

    Aabb MakeAabbFromCenterExtent(const math::vec3 &center, const math::vec3 &ext) noexcept {
        return Aabb{center - ext, center + ext};
    }

    Aabb BoundingAabb(const Sphere &s) noexcept {
        const math::vec3 radius_vec{s.radius};
        return Aabb{s.center - radius_vec, s.center + radius_vec};
    }

    Aabb BoundingAabb(const Obb &s) noexcept {
        const auto corner_points = GetCorners(s);

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

        for (const auto &corner: corner_points) {
            for (std::size_t i = 0; i < 3; ++i) {
                min_corner[i] = math::utils::min(min_corner[i], corner[i]);
                max_corner[i] = math::utils::max(max_corner[i], corner[i]);
            }
        }

        return Aabb{min_corner, max_corner};
    }

    Aabb BoundingAabb(const Cylinder &s) noexcept {
        const math::vec3 dir = AxisDirection(s);
        const math::vec3 abs_dir{std::fabs(dir[0]), std::fabs(dir[1]), std::fabs(dir[2])};

        math::vec3 axial_extent = abs_dir * s.half_height;
        math::vec3 radial_extent{};
        for (std::size_t i = 0; i < 3; ++i) {
            const float component = std::max(0.0f, 1.0f - abs_dir[i] * abs_dir[i]);
            radial_extent[i] = s.radius * std::sqrt(component);
        }

        const math::vec3 extent = axial_extent + radial_extent;
        return MakeAabbFromCenterExtent(s.center, extent);
    }

    Aabb BoundingAabb(const Ellipsoid &s) noexcept {
        math::vec3 extent{0.0f};
        for (std::size_t row = 0; row < 3; ++row) {
            float sum = 0.0f;
            for (std::size_t col = 0; col < 3; ++col) {
                sum += std::fabs(s.orientation[row][col]) * s.radii[col];
            }
            extent[row] = sum;
        }

        return MakeAabbFromCenterExtent(s.center, extent);
    }

    Aabb BoundingAabb(const Segment &s) noexcept {
        math::vec3 min_corner{};
        math::vec3 max_corner{};
        for (std::size_t i = 0; i < 3; ++i) {
            min_corner[i] = math::utils::min(s.start[i], s.end[i]);
            max_corner[i] = math::utils::max(s.start[i], s.end[i]);
        }
        return Aabb{min_corner, max_corner};
    }

    Aabb BoundingAabb(const Triangle &s) noexcept {
        math::vec3 min_corner{
            math::utils::min(s.a[0], math::utils::min(s.b[0], s.c[0])),
            math::utils::min(s.a[1], math::utils::min(s.b[1], s.c[1])),
            math::utils::min(s.a[2], math::utils::min(s.b[2], s.c[2]))
        };

        math::vec3 max_corner{
            math::utils::max(s.a[0], math::utils::max(s.b[0], s.c[0])),
            math::utils::max(s.a[1], math::utils::max(s.b[1], s.c[1])),
            math::utils::max(s.a[2], math::utils::max(s.b[2], s.c[2]))
        };

        return Aabb{min_corner, max_corner};
    }

    Aabb Merge(const Aabb &l, const Aabb &r) noexcept {
        Aabb result = l;
        Merge(result, r);
        return result;
    }

    void Merge(Aabb &box, const Aabb &other) noexcept {
        for (std::size_t i = 0; i < 3; ++i) {
            box.min[i] = math::utils::min(box.min[i], other.min[i]);
            box.max[i] = math::utils::max(box.max[i], other.max[i]);
        }
    }

    void Merge(Aabb &box, const math::vec3 &point) noexcept {
        for (std::size_t i = 0; i < 3; ++i) {
            box.min[i] = math::utils::min(box.min[i], point[i]);
            box.max[i] = math::utils::max(box.max[i], point[i]);
        }
    }

    [[nodiscard]] math::vec3 ClosestPoint(const Aabb &box, const math::vec3 &point) noexcept {
        return {
            math::utils::clamp(point[0], box.min[0], box.max[0]),
            math::utils::clamp(point[1], box.min[1], box.max[1]),
            math::utils::clamp(point[2], box.min[2], box.max[2])
        };
    }

    [[nodiscard]] double SquaredDistance(const Aabb &box, const math::vec3 &point) noexcept {
        const double dx = math::utils::max(math::utils::max(box.min[0] - point[0], 0.0f), point[0] - box.max[0]);
        const double dy = math::utils::max(math::utils::max(box.min[1] - point[1], 0.0f), point[1] - box.max[1]);
        const double dz = math::utils::max(math::utils::max(box.min[2] - point[2], 0.0f), point[2] - box.max[2]);
        return dx * dx + dy * dy + dz * dz;
    }

    [[nodiscard]] std::array<math::vec3, 8> GetCorners(const Aabb &box) noexcept {
        std::array<math::vec3, 8> v{};
        for (int i = 0; i < 8; ++i) {
            v[i] = math::vec3{
                (i & 1) ? box.max[0] : box.min[0],
                (i & 2) ? box.max[1] : box.min[1],
                (i & 4) ? box.max[2] : box.min[2]
            };
        }
        return v;
    }

    [[nodiscard]] std::array<math::ivec2, 12> GetEdges(const Aabb &) noexcept {
        return {
            {
                {0, 1}, {1, 3}, {3, 2}, {2, 0}, // Bottom face edges
                {4, 5}, {5, 7}, {7, 6}, {6, 4}, // Top face edges
                {0, 4}, {1, 5}, {2, 6}, {3, 7} // Vertical edges
            }
        };
    }

    [[nodiscard]] std::array<math::ivec3, 12> GetFaceTriangles(const Aabb &) noexcept {
        return {
            {
                // -X
                {0, 4, 6}, {0, 6, 2},
                // +X
                {1, 3, 7}, {1, 7, 5},
                // -Y
                {0, 1, 5}, {0, 5, 4},
                // +Y
                {2, 6, 7}, {2, 7, 3},
                // -Z (bottom)
                {0, 2, 3}, {0, 3, 1},
                // +Z (top)
                {4, 5, 7}, {4, 7, 6}
            }
        };
    }

    [[nodiscard]] std::array<math::ivec4, 6> GetFaceQuads(const Aabb &) noexcept {
        return {
            {
                {0, 4, 6, 2}, // -X
                {1, 3, 7, 5}, // +X
                {0, 1, 5, 4}, // -Y
                {2, 6, 7, 3}, // +Y
                {0, 2, 3, 1}, // -Z (bottom)
                {4, 5, 7, 6} // +Z (top)
            }
        };
    }
} // namespace engine::geometry
