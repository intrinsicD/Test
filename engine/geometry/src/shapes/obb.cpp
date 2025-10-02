#include "engine/geometry/shapes.hpp"
#include "engine/math/matrix.hpp"

#include <array>
#include <cmath>


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
                box.half_sizes[0] * box.half_sizes[2]) * 4.0;
    }

    double Volume(const Obb &box) noexcept {
        return box.half_sizes[0] * box.half_sizes[1] * box.half_sizes[2] * 2;
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
        //TODO
    }

    Obb BoundingObb(std::span<math::vec3> points) noexcept {
        //TODO
    }

    Obb FromCenterHalfSizes(const math::vec3 &center, const math::vec3 &half_sizes) noexcept {
        return Obb{center, half_sizes, {}};
    }

    math::vec3 ClosestPoint(const Obb &obb, const math::vec3 &point) noexcept {
        //TODO: Is this correct?
        math::mat3 R = obb.orientation.to_rotation_matrix();
        math::vec3 point_rel = math::inverse(R) * point - obb.center;
        math::vec3 point_closest = ClosestPoint(BoundingAabb(obb), math::vec3{point_rel});
        return R * point_closest;
    }

    double SquaredDistance(const Obb &box, const math::vec3 &point) noexcept {
        //TODO: Is this correct? or should i use the aabb version and transform the point?
        const math::vec3 closest = ClosestPoint(box, point);
        return math::length_squared(point - closest);
    }

    std::array<math::vec3, 8> GetCorners(const Obb &box) noexcept {
        std::array<math::vec3, 8> vertices{};
        math::mat3 R = box.orientation.to_rotation_matrix();
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
