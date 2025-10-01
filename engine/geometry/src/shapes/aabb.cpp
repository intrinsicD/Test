#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/shapes/triangle.hpp"
#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/math/utils.hpp"


#include <algorithm>
#include <array>
#include <cmath>
#include <engine/geometry/shapes/segment.hpp>


namespace engine::geometry {
    namespace {
        [[nodiscard]] constexpr float half() noexcept { return 0.5f; }
        [[nodiscard]] constexpr float two() noexcept { return 2.0f; }

        [[nodiscard]] std::array<math::vec3, 8> Corners(const Obb &box) noexcept {
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
    } // namespace

    math::vec3 Center(const Aabb &box) noexcept {
        return (box.min + box.max) * half();
    }

    math::vec3 Size(const Aabb &box) noexcept {
        return box.max - box.min;
    }

    math::vec3 Extent(const Aabb &box) noexcept {
        return Size(box) * half();
    }

    float SurfaceArea(const Aabb &box) noexcept {
        const math::vec3 s = Size(box);
        return two() * (s[0] * s[1] + s[1] * s[2] + s[0] * s[2]);
    }

    float Volume(const Aabb &box) noexcept {
        const math::vec3 s = Size(box);
        return s[0] * s[1] * s[2];
    }

    bool Contains(const Aabb &box, const math::vec3 &point) noexcept {
        for (std::size_t i = 0; i < 3; ++i) {
            if (point[i] < box.min[i] || point[i] > box.max[i]) {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Aabb &outer, const Aabb &inner) noexcept {
        for (std::size_t i = 0; i < 3; ++i) {
            if (inner.min[i] < outer.min[i] || inner.max[i] > outer.max[i]) {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Aabb &outer, const Sphere &inner) noexcept {
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

    bool Contains(const Aabb &outer, const Obb &inner) noexcept {
        for (const auto &corner: Corners(inner)) {
            if (!Contains(outer, corner)) {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Aabb &outer, const Cylinder &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Ellipsoid &inner) noexcept {
        //TODO
    }

    bool Contains(const Aabb &outer, const Segment &inner) noexcept {
        return Contains(outer, inner.start) && Contains(outer, inner.end);
    }

    bool Contains(const Aabb &outer, const Triangle &inner) noexcept {
        return Contains(outer, inner.a) && Contains(outer, inner.b) && Contains(outer, inner.c);
    }

    bool Intersects(const Aabb &lhs, const Aabb &rhs) noexcept {
        for (std::size_t i = 0; i < 3; ++i) {
            if (lhs.max[i] < rhs.min[i] || lhs.min[i] > rhs.max[i]) {
                return false;
            }
        }
        return true;
    }

    bool Intersects(const Aabb &box, const Sphere &s) noexcept {
        math::vec3 closest{};
        for (std::size_t i = 0; i < 3; ++i) {
            closest[i] = math::utils::clamp(s.center[i], box.min[i], box.max[i]);
        }
        const math::vec3 offset = closest - s.center;
        return math::length_squared(offset) <= s.radius * s.radius;
    }

    bool Intersects(const Aabb &box, const Obb &other) noexcept {
        return Intersects(other, box);
    }

    bool Intersects(const Aabb &box, const Cylinder &other) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &box, const Ellipsoid &other) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &box, const Line &other) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &box, const Plane &other) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &box, const Ray &other) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &box, const Segment &other) noexcept {
        //TODO
    }

    bool Intersects(const Aabb &box, const Triangle &other) noexcept {
        //TODO
    }

    Aabb make_aabb_from_point(const math::vec3 &point) noexcept {
        return Aabb{point, point};
    }

    Aabb make_aabb_from_center_extent(const math::vec3 &center, const math::vec3 &ext) noexcept {
        return Aabb{center - ext, center + ext};
    }

    Aabb BoundingAabb(const Sphere &s) noexcept {
        const math::vec3 radius_vec{s.radius};
        return Aabb{s.center - radius_vec, s.center + radius_vec};
    }

    Aabb BoundingAabb(const Obb &s) noexcept {
        const auto corner_points = Corners(s);

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
        //TODO
    }

    Aabb BoundingAabb(const Ellipsoid &s) noexcept {
        //TODO
    }

    Aabb BoundingAabb(const Segment &s) noexcept {
        //TODO
    }

    Aabb BoundingAabb(const Triangle &s) noexcept {
        //TODO
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

    [[nodiscard]] std::array<math::vec3, 8> Corners(const Aabb &box) noexcept {
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

    [[nodiscard]] std::array<math::ivec2, 12> Edges(const Aabb &) noexcept {
        return {
            {
                {0, 1}, {1, 3}, {3, 2}, {2, 0}, // Bottom face edges
                {4, 5}, {5, 7}, {7, 6}, {6, 4}, // Top face edges
                {0, 4}, {1, 5}, {2, 6}, {3, 7} // Vertical edges
            }
        };
    }

    [[nodiscard]] std::array<math::ivec3, 12> FaceTriangles(const Aabb &) noexcept {
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

    [[nodiscard]] std::array<math::ivec4, 6> FaceQuads(const Aabb &) noexcept {
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
