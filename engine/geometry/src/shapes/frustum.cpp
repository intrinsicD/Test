#include "engine/geometry/shapes/frustum.hpp"
#include "engine/math/vector.hpp"

#include <cmath>
#include <limits>

namespace engine::geometry {

    Frustum ExtractFrustum(const math::mat4& vp) noexcept {
        Frustum frustum;

        // Extract frustum planes from view-projection matrix using the Gribb-Hartmann method.
        // The math::mat4 stores data in column-major order, so we operate on columns directly
        // (equivalent to taking the planes of the clip-space frustum and transforming them by
        // the inverse-transpose of the view-projection matrix).

        const math::vec4 column0 = vp.col(0);
        const math::vec4 column1 = vp.col(1);
        const math::vec4 column2 = vp.col(2);
        const math::vec4 column3 = vp.col(3);

        const auto assign_plane = [](Plane& plane, const math::vec4& coefficients) {
            plane.normal = math::vec3{coefficients[0], coefficients[1], coefficients[2]};
            plane.distance = coefficients[3];
        };

        assign_plane(frustum.planes[Frustum::kLeft], column3 + column0);
        assign_plane(frustum.planes[Frustum::kRight], column3 - column0);
        assign_plane(frustum.planes[Frustum::kBottom], column3 + column1);
        assign_plane(frustum.planes[Frustum::kTop], column3 - column1);
        assign_plane(frustum.planes[Frustum::kNear], column3 + column2);
        assign_plane(frustum.planes[Frustum::kFar], column3 - column2);

        // Normalize all planes so SignedDistance can rely on unit-length normals.
        for (auto& plane : frustum.planes) {
            const float length = math::length(plane.normal);
            if (length > std::numeric_limits<float>::epsilon()) {
                const float inv_length = 1.0f / length;
                plane.normal = plane.normal * inv_length;
                plane.distance *= inv_length;
            }
        }

        return frustum;
    }

    std::array<math::vec3, 8> GetCorners(const Frustum& frustum) noexcept {
        std::array<math::vec3, 8> corners;

        // Compute frustum corners by intersecting plane triplets
        auto intersect_three_planes = [](const Plane& p1, const Plane& p2, const Plane& p3) -> math::vec3 {
            // Solve: n1·x + d1 = 0, n2·x + d2 = 0, n3·x + d3 = 0
            const math::vec3 n1xn2 = math::cross(p1.normal, p2.normal);
            const float det = math::dot(n1xn2, p3.normal);

            if (std::abs(det) < std::numeric_limits<float>::epsilon()) {
                return math::vec3{0.0f, 0.0f, 0.0f};
            }

            const math::vec3 n2xn3 = math::cross(p2.normal, p3.normal);
            const math::vec3 n3xn1 = math::cross(p3.normal, p1.normal);

            return (n2xn3 * (-p1.distance) + n3xn1 * (-p2.distance) + n1xn2 * (-p3.distance)) * (1.0f / det);
        };

        const auto& left = frustum.planes[Frustum::kLeft];
        const auto& right = frustum.planes[Frustum::kRight];
        const auto& bottom = frustum.planes[Frustum::kBottom];
        const auto& top = frustum.planes[Frustum::kTop];
        const auto& near = frustum.planes[Frustum::kNear];
        const auto& far = frustum.planes[Frustum::kFar];

        // Near plane corners
        corners[0] = intersect_three_planes(left, bottom, near);
        corners[1] = intersect_three_planes(right, bottom, near);
        corners[2] = intersect_three_planes(right, top, near);
        corners[3] = intersect_three_planes(left, top, near);

        // Far plane corners
        corners[4] = intersect_three_planes(left, bottom, far);
        corners[5] = intersect_three_planes(right, bottom, far);
        corners[6] = intersect_three_planes(right, top, far);
        corners[7] = intersect_three_planes(left, top, far);

        return corners;
    }

} // namespace engine::geometry

