#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/math/utils.hpp"

#include <cmath>
#include <numbers>


namespace engine::geometry
{
    namespace
    {
        [[nodiscard]] constexpr float two() noexcept { return 2.0f; }
    } // namespace

    math::vec3 AxisDirection(const Cylinder& cylinder) noexcept
    {
        const float len = math::length(cylinder.axis);
        if (len == 0.0f)
        {
            return math::vec3{0.0f};
        }
        return cylinder.axis / len;
    }

    math::vec3 TopCenter(const Cylinder& cylinder) noexcept
    {
        return cylinder.center + AxisDirection(cylinder) * cylinder.half_height;
    }

    math::vec3 BottomCenter(const Cylinder& cylinder) noexcept
    {
        return cylinder.center - AxisDirection(cylinder) * cylinder.half_height;
    }

    float Volume(const Cylinder& cylinder) noexcept
    {
        const float height = cylinder.half_height * two();
        return std::numbers::pi_v<float> * cylinder.radius * cylinder.radius * height;
    }

    float LateralSurfaceArea(const Cylinder& cylinder) noexcept
    {
        const float height = cylinder.half_height * two();
        return two() * std::numbers::pi_v<float> * cylinder.radius * height;
    }

    float SurfaceArea(const Cylinder& cylinder) noexcept
    {
        return LateralSurfaceArea(cylinder) + two() * std::numbers::pi_v<float> * cylinder.radius * cylinder.radius;
    }

    math::vec3 ClosestPoint(const Cylinder& cylinder, const math::vec3& point) noexcept
    {
        const math::vec3 axis_dir = AxisDirection(cylinder);
        const math::vec3 to_point = point - cylinder.center;
        
        // Project point onto cylinder axis
        const float axis_proj = math::dot(to_point, axis_dir);
        const float clamped_proj = math::utils::clamp(axis_proj, -cylinder.half_height, cylinder.half_height);
        
        // Point on axis at clamped projection
        const math::vec3 axis_point = cylinder.center + clamped_proj * axis_dir;
        
        // Vector from axis to point (perpendicular to axis)
        const math::vec3 radial = point - axis_point;
        const float radial_dist_sq = math::length_squared(radial);
        
        // If point is inside cylinder (radially), return the point itself if within height
        if (radial_dist_sq <= cylinder.radius * cylinder.radius)
        {
            // Point is inside cylindrical volume
            if (std::abs(axis_proj) <= cylinder.half_height)
            {
                return point;
            }
            // Point is beyond caps but within radius - return point on cap
            return axis_point;
        }
        
        // Point is outside radius - project onto lateral surface or cap edge
        const math::vec3 radial_dir = radial / std::sqrt(radial_dist_sq);
        const math::vec3 surface_point = axis_point + radial_dir * cylinder.radius;
        
        return surface_point;
    }

    double SquaredDistance(const Cylinder& cylinder, const math::vec3& point) noexcept
    {
        const math::vec3 closest = ClosestPoint(cylinder, point);
        return math::length_squared(point - closest);
    }
} // namespace engine::geometry
