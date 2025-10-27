#include "engine/geometry/shapes/frustum.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace engine::geometry
{
    namespace
    {
        using CornerArray = std::array<math::vec3, 8>;

        [[nodiscard]] CornerArray UnprojectCorners(const math::mat4& vp) noexcept
        {
            CornerArray corners{};

            const auto vp_d = math::cast<double>(vp);
            const auto inverse_opt = math::try_inverse(vp_d);
            if (!inverse_opt)
            {
                return corners;
            }

            const auto inverse = *inverse_opt;
            std::size_t index = 0;
            for (int z = -1; z <= 1; z += 2)
            {
                for (int y = -1; y <= 1; y += 2)
                {
                    for (int x = -1; x <= 1; x += 2)
                    {
                        const math::Vector<double, 4> clip_corner{
                            static_cast<double>(x),
                            static_cast<double>(y),
                            static_cast<double>(z),
                            1.0
                        };
                        const auto world_h = inverse * clip_corner;
                        const double w = world_h[3];
                        if (std::abs(w) > std::numeric_limits<double>::epsilon())
                        {
                            const double inv_w = 1.0 / w;
                            corners[index] = math::vec3{
                                static_cast<float>(world_h[0] * inv_w),
                                static_cast<float>(world_h[1] * inv_w),
                                static_cast<float>(world_h[2] * inv_w)
                            };
                        }
                        else
                        {
                            corners[index] = math::vec3{0.0f, 0.0f, 0.0f};
                        }
                        ++index;
                    }
                }
            }

            return corners;
        }

        [[nodiscard]] Plane MakePlane(const math::vec3& a,
                                      const math::vec3& b,
                                      const math::vec3& c,
                                      const math::vec3& center) noexcept
        {
            math::vec3 normal = math::cross(b - a, c - a);
            const float length = math::length(normal);
            if (length <= std::numeric_limits<float>::epsilon())
            {
                return Plane{math::vec3{0.0f, 0.0f, 0.0f}, 0.0f};
            }

            normal *= 1.0f / length;
            float distance = -math::dot(normal, a);
            const math::vec3 to_center = center - a;
            if (math::dot(normal, to_center) < 0.0f)
            {
                normal = -normal;
                distance = -distance;
            }

            return Plane{normal, distance};
        }
    } // namespace

    Frustum ExtractFrustum(const math::mat4& vp) noexcept
    {
        Frustum frustum;

        const CornerArray corners = UnprojectCorners(vp);

        math::vec3 center{0.0f};
        for (const auto& corner : corners)
        {
            center += corner;
        }
        center *= 1.0f / static_cast<float>(corners.size());

        const auto plane_from_indices = [&](std::size_t i0, std::size_t i1, std::size_t i2)
        {
            return MakePlane(corners[i0], corners[i1], corners[i2], center);
        };

        // Corner ordering: 0=NBL, 1=NBR, 2=NTL, 3=NTR, 4=FBL, 5=FBR, 6=FTL, 7=FTR.
        frustum.planes[Frustum::kLeft] = plane_from_indices(0, 4, 2);
        frustum.planes[Frustum::kRight] = plane_from_indices(1, 3, 5);
        frustum.planes[Frustum::kBottom] = plane_from_indices(0, 5, 1);
        frustum.planes[Frustum::kTop] = plane_from_indices(2, 6, 3);
        frustum.planes[Frustum::kNear] = plane_from_indices(0, 1, 3);
        frustum.planes[Frustum::kFar] = plane_from_indices(4, 7, 6);

        return frustum;
    }

    std::array<math::vec3, 8> GetCorners(const Frustum& frustum) noexcept
    {
        std::array<math::vec3, 8> corners;

        // Compute frustum corners by intersecting plane triplets
        auto intersect_three_planes = [](const Plane& p1, const Plane& p2, const Plane& p3) -> math::vec3
        {
            // Solve: n1·x + d1 = 0, n2·x + d2 = 0, n3·x + d3 = 0
            const math::vec3 n1xn2 = math::cross(p1.normal, p2.normal);
            const float det = math::dot(n1xn2, p3.normal);

            if (std::abs(det) < std::numeric_limits<float>::epsilon())
            {
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