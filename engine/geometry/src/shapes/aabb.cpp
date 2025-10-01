#include "engine/geometry/shapes/aabb.hpp"

#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/geometry/shapes/ellipsoid.hpp"
#include "engine/geometry/shapes/line.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/geometry/shapes/ray.hpp"
#include "engine/geometry/shapes/segment.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/shapes/triangle.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>


namespace engine::geometry {
    namespace {
        [[nodiscard]] constexpr float half() noexcept { return 0.5f; }
        [[nodiscard]] constexpr float two() noexcept { return 2.0f; }
        [[nodiscard]] constexpr float tiny() noexcept { return 1e-6f; }

        [[nodiscard]] Segment axis_segment(const Cylinder &c) noexcept {
            const math::vec3 dir = AxisDirection(c);
            const math::vec3 offset = dir * c.half_height;
            return Segment{c.center - offset, c.center + offset};
        }

        [[nodiscard]] double squared_distance(const Aabb &box, const Segment &segment) noexcept {
            const math::vec3 direction = segment.end - segment.start;
            const double dir_length_sq = static_cast<double>(math::length_squared(direction));

            std::array<float, 32> candidates{};
            std::size_t candidate_count = 0U;

            const auto add_candidate = [&](float value) {
                if (value < 0.0f || value > 1.0f) {
                    return;
                }
                for (std::size_t i = 0; i < candidate_count; ++i) {
                    if (std::fabs(candidates[i] - value) <= 1e-4f) {
                        return;
                    }
                }
                candidates[candidate_count++] = value;
            };

            add_candidate(0.0f);
            add_candidate(1.0f);

            if (dir_length_sq > tiny()) {
                const float projected = math::dot(Center(box) - segment.start, direction) /
                                       static_cast<float>(dir_length_sq);
                add_candidate(projected);

                for (std::size_t axis = 0; axis < 3; ++axis) {
                    if (std::fabs(direction[axis]) <= tiny()) {
                        continue;
                    }
                    const float inv_dir = 1.0f / direction[axis];
                    add_candidate((box.min[axis] - segment.start[axis]) * inv_dir);
                    add_candidate((box.max[axis] - segment.start[axis]) * inv_dir);
                }

                for (const auto &corner : GetCorners(box)) {
                    const float numerator = math::dot(corner - segment.start, direction);
                    const float t = numerator / static_cast<float>(dir_length_sq);
                    add_candidate(t);
                }
            }

            double min_distance_sq = std::numeric_limits<double>::infinity();
            for (std::size_t i = 0; i < candidate_count; ++i) {
                const math::vec3 point = segment.start + direction * candidates[i];
                min_distance_sq = std::min(min_distance_sq, SquaredDistance(box, point));
            }

            return min_distance_sq;
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

    double SurfaceArea(const Aabb &box) noexcept {
        const math::vec3 s = Size(box);
        return two() * (s[0] * s[1] + s[1] * s[2] + s[0] * s[2]);
    }

    double Volume(const Aabb &box) noexcept {
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
        for (const auto &corner: GetCorners(inner)) {
            if (!Contains(outer, corner)) {
                return false;
            }
        }
        return true;
    }

    bool Contains(const Aabb &outer, const Cylinder &inner) noexcept {
        return Contains(outer, BoundingAabb(inner));
    }

    bool Contains(const Aabb &outer, const Ellipsoid &inner) noexcept {
        return Contains(outer, BoundingAabb(inner));
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
        const Segment axis = axis_segment(other);
        const math::vec3 direction = axis.end - axis.start;
        const float dir_length_sq = math::length_squared(direction);

        if (dir_length_sq <= tiny()) {
            return Intersects(box, Sphere{other.center, other.radius});
        }

        float t_min = 0.0f;
        float t_max = 0.0f;
        const Ray axis_ray{axis.start, direction};
        if (Intersects(axis_ray, box, t_min, t_max) && t_max >= 0.0f && t_min <= 1.0f) {
            return true;
        }

        const double distance_sq = squared_distance(box, axis);
        const double radius_sq = static_cast<double>(other.radius) * static_cast<double>(other.radius);
        return distance_sq <= radius_sq + 1e-6;
    }

    bool Intersects(const Aabb &box, const Ellipsoid &other) noexcept {
        if (Contains(box, other.center)) {
            return true;
        }

        for (const auto &corner : GetCorners(box)) {
            if (Contains(other, corner)) {
                return true;
            }
        }

        for (std::size_t i = 0; i < 3; ++i) {
            if (other.radii[i] <= tiny()) {
                return Contains(box, other.center);
            }
        }

        const math::mat3 inv_radii_sq{
            1.0f / (other.radii[0] * other.radii[0]), 0.0f, 0.0f,
            0.0f, 1.0f / (other.radii[1] * other.radii[1]), 0.0f,
            0.0f, 0.0f, 1.0f / (other.radii[2] * other.radii[2])
        };
        const math::mat3 metric = other.orientation * inv_radii_sq * math::transpose(other.orientation);

        math::vec3 point{
            math::utils::clamp(other.center[0], box.min[0], box.max[0]),
            math::utils::clamp(other.center[1], box.min[1], box.max[1]),
            math::utils::clamp(other.center[2], box.min[2], box.max[2])
        };

        for (int iteration = 0; iteration < 16; ++iteration) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                float sum = 0.0f;
                for (std::size_t j = 0; j < 3; ++j) {
                    if (j == axis) {
                        continue;
                    }
                    sum += metric[axis][j] * (point[j] - other.center[j]);
                }
                const float diag = metric[axis][axis];
                if (std::fabs(diag) <= tiny()) {
                    continue;
                }
                const float target = other.center[axis] - sum / diag;
                point[axis] = math::utils::clamp(target, box.min[axis], box.max[axis]);
            }
        }

        const math::vec3 diff = point - other.center;
        const math::vec3 transformed = metric * diff;
        const float value = math::dot(diff, transformed);
        return value <= 1.0f + 1e-4f;
    }

    bool Intersects(const Aabb &box, const Line &other) noexcept {
        float t_min = -std::numeric_limits<float>::infinity();
        float t_max = std::numeric_limits<float>::infinity();

        for (std::size_t axis = 0; axis < 3; ++axis) {
            const float dir = other.direction[axis];
            const float origin = other.point[axis];

            if (std::fabs(dir) <= tiny()) {
                if (origin < box.min[axis] || origin > box.max[axis]) {
                    return false;
                }
                continue;
            }

            const float inv_dir = 1.0f / dir;
            float t0 = (box.min[axis] - origin) * inv_dir;
            float t1 = (box.max[axis] - origin) * inv_dir;
            if (t0 > t1) {
                std::swap(t0, t1);
            }

            t_min = std::max(t_min, t0);
            t_max = std::min(t_max, t1);
            if (t_max < t_min) {
                return false;
            }
        }

        return true;
    }

    bool Intersects(const Aabb &box, const Plane &other) noexcept {
        const math::vec3 center = Center(box);
        const math::vec3 extent = Extent(box);

        const float distance = math::dot(other.normal, center) + other.distance;
        const float radius = extent[0] * std::fabs(other.normal[0]) +
                             extent[1] * std::fabs(other.normal[1]) +
                             extent[2] * std::fabs(other.normal[2]);
        return std::fabs(distance) <= radius;
    }

    bool Intersects(const Aabb &box, const Ray &other) noexcept {
        float t_min = 0.0f;
        float t_max = 0.0f;
        return Intersects(other, box, t_min, t_max);
    }

    bool Intersects(const Aabb &box, const Segment &other) noexcept {
        const math::vec3 direction = other.end - other.start;
        const float dir_length_sq = math::length_squared(direction);
        if (dir_length_sq <= tiny()) {
            return Contains(box, other.start);
        }

        float t_min = 0.0f;
        float t_max = 0.0f;
        const Ray ray{other.start, direction};
        if (!Intersects(ray, box, t_min, t_max)) {
            return false;
        }
        return t_max >= 0.0f && t_min <= 1.0f;
    }

    bool Intersects(const Aabb &box, const Triangle &other) noexcept {
        const math::vec3 box_center = Center(box);
        const math::vec3 box_extent = Extent(box);

        const math::vec3 v0 = other.a - box_center;
        const math::vec3 v1 = other.b - box_center;
        const math::vec3 v2 = other.c - box_center;

        const math::vec3 f0 = v1 - v0;
        const math::vec3 f1 = v2 - v1;
        const math::vec3 f2 = v0 - v2;

        const auto axis_test = [&](const math::vec3 &axis) {
            if (math::length_squared(axis) <= tiny()) {
                return true;
            }
            const float p0 = math::dot(v0, axis);
            const float p1 = math::dot(v1, axis);
            const float p2 = math::dot(v2, axis);
            const float min_p = std::min({p0, p1, p2});
            const float max_p = std::max({p0, p1, p2});
            const float r = box_extent[0] * std::fabs(axis[0]) +
                            box_extent[1] * std::fabs(axis[1]) +
                            box_extent[2] * std::fabs(axis[2]);
            return !(min_p > r || max_p < -r);
        };

        const math::vec3 axes[3] = {f0, f1, f2};
        for (const auto &edge : axes) {
            if (!axis_test(math::cross(edge, math::vec3{1.0f, 0.0f, 0.0f}))) {
                return false;
            }
            if (!axis_test(math::cross(edge, math::vec3{0.0f, 1.0f, 0.0f}))) {
                return false;
            }
            if (!axis_test(math::cross(edge, math::vec3{0.0f, 0.0f, 1.0f}))) {
                return false;
            }
        }

        for (std::size_t axis = 0; axis < 3; ++axis) {
            const float min_v = std::min({v0[axis], v1[axis], v2[axis]});
            const float max_v = std::max({v0[axis], v1[axis], v2[axis]});
            if (min_v > box_extent[axis] || max_v < -box_extent[axis]) {
                return false;
            }
        }

        const math::vec3 normal_vec = math::cross(f0, f1);
        if (!axis_test(normal_vec)) {
            return false;
        }

        return true;
    }

    Aabb BoundingAabb(const math::vec3 &point) noexcept {
        return Aabb{point, point};
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
