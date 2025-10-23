#include "engine/geometry/shapes/frustum.hpp"
#include "engine/math/vector.hpp"

#include <cmath>
#include <limits>

namespace engine::geometry {

    Frustum ExtractFrustum(const math::mat4& vp) noexcept {
        Frustum frustum;

        // Extract frustum planes from view-projection matrix using Gribb-Hartmann method
        // Matrix is column-major: vp[col][row]
        // Each plane equation is derived from the homogeneous clip space: -w <= x,y,z <= w

        // Left plane: row3 + row0
        frustum.planes[Frustum::kLeft].normal = math::vec3{
            vp[3][0] + vp[0][0],
            vp[3][1] + vp[0][1],
            vp[3][2] + vp[0][2]
        };
        frustum.planes[Frustum::kLeft].distance = vp[3][3] + vp[0][3];

        // Right plane: row3 - row0
        frustum.planes[Frustum::kRight].normal = math::vec3{
            vp[3][0] - vp[0][0],
            vp[3][1] - vp[0][1],
            vp[3][2] - vp[0][2]
        };
        frustum.planes[Frustum::kRight].distance = vp[3][3] - vp[0][3];

        // Bottom plane: row3 + row1
        frustum.planes[Frustum::kBottom].normal = math::vec3{
            vp[3][0] + vp[1][0],
            vp[3][1] + vp[1][1],
            vp[3][2] + vp[1][2]
        };
        frustum.planes[Frustum::kBottom].distance = vp[3][3] + vp[1][3];

        // Top plane: row3 - row1
        frustum.planes[Frustum::kTop].normal = math::vec3{
            vp[3][0] - vp[1][0],
            vp[3][1] - vp[1][1],
            vp[3][2] - vp[1][2]
        };
        frustum.planes[Frustum::kTop].distance = vp[3][3] - vp[1][3];

        // Near plane: row3 + row2
        frustum.planes[Frustum::kNear].normal = math::vec3{
            vp[3][0] + vp[2][0],
            vp[3][1] + vp[2][1],
            vp[3][2] + vp[2][2]
        };
        frustum.planes[Frustum::kNear].distance = vp[3][3] + vp[2][3];

        // Far plane: row3 - row2
        frustum.planes[Frustum::kFar].normal = math::vec3{
            vp[3][0] - vp[2][0],
            vp[3][1] - vp[2][1],
            vp[3][2] - vp[2][2]
        };
        frustum.planes[Frustum::kFar].distance = vp[3][3] - vp[2][3];

        // Normalize all planes
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

