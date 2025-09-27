#pragma once

#include <cstddef>

#include "engine/geometry/api.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry {

struct aabb {
    math::vec3 min;
    math::vec3 max;
};

[[nodiscard]] math::vec3 center(const aabb& box) noexcept;
[[nodiscard]] math::vec3 size(const aabb& box) noexcept;
[[nodiscard]] math::vec3 extent(const aabb& box) noexcept;
[[nodiscard]] float surface_area(const aabb& box) noexcept;
[[nodiscard]] float volume(const aabb& box) noexcept;
[[nodiscard]] bool contains(const aabb& box, const math::vec3& point) noexcept;

struct obb {
    math::vec3 center;
    math::vec3 half_sizes;
    math::mat3 orientation;
};

[[nodiscard]] math::vec3 size(const obb& box) noexcept;
[[nodiscard]] math::vec3 extent(const obb& box) noexcept;
[[nodiscard]] bool contains(const obb& box, const math::vec3& point) noexcept;
[[nodiscard]] aabb bounding_aabb(const obb& box) noexcept;

struct sphere {
    math::vec3 center;
    float radius;
};

[[nodiscard]] float surface_area(const sphere& s) noexcept;
[[nodiscard]] float volume(const sphere& s) noexcept;
[[nodiscard]] bool contains(const sphere& s, const math::vec3& point) noexcept;

struct plane {
    math::vec3 normal;
    float distance;
};

[[nodiscard]] float signed_distance(const plane& p, const math::vec3& point) noexcept;
[[nodiscard]] math::vec3 project_point(const plane& p, const math::vec3& point) noexcept;
[[nodiscard]] bool contains(const plane& p, const math::vec3& point, float epsilon = 1e-4f) noexcept;

struct ray {
    math::vec3 origin;
    math::vec3 direction;
};

[[nodiscard]] math::vec3 point_at(const ray& r, float t) noexcept;

struct segment {
    math::vec3 start;
    math::vec3 end;
};

[[nodiscard]] math::vec3 direction(const segment& s) noexcept;
[[nodiscard]] float length(const segment& s) noexcept;
[[nodiscard]] math::vec3 point_at(const segment& s, float t) noexcept;

struct line {
    math::vec3 point;
    math::vec3 direction;
};

[[nodiscard]] math::vec3 point_at(const line& l, float t) noexcept;
[[nodiscard]] math::vec3 project_point(const line& l, const math::vec3& point) noexcept;

struct ellipsoid {
    math::vec3 center;
    math::vec3 radii;
    math::mat3 orientation;
};

[[nodiscard]] float volume(const ellipsoid& e) noexcept;
[[nodiscard]] bool contains(const ellipsoid& e, const math::vec3& point) noexcept;

struct triangle {
    math::vec3 a;
    math::vec3 b;
    math::vec3 c;
};

[[nodiscard]] math::vec3 normal(const triangle& t) noexcept;
[[nodiscard]] math::vec3 unit_normal(const triangle& t) noexcept;
[[nodiscard]] float area(const triangle& t) noexcept;
[[nodiscard]] math::vec3 centroid(const triangle& t) noexcept;

struct cylinder {
    math::vec3 center;
    math::vec3 axis;
    float radius;
    float half_height;
};

[[nodiscard]] math::vec3 axis_direction(const cylinder& c) noexcept;
[[nodiscard]] math::vec3 top_center(const cylinder& c) noexcept;
[[nodiscard]] math::vec3 bottom_center(const cylinder& c) noexcept;
[[nodiscard]] float volume(const cylinder& c) noexcept;
[[nodiscard]] float lateral_surface_area(const cylinder& c) noexcept;
[[nodiscard]] float surface_area(const cylinder& c) noexcept;
[[nodiscard]] bool contains(const cylinder& c, const math::vec3& point) noexcept;

}  // namespace engine::geometry

