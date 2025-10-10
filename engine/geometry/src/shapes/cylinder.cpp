#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "../../../math/include/engine/math/utils/utils.hpp"

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

        // Vector from axis to point, guaranteed to be orthogonal to the axis direction
        const math::vec3 radial = to_point - axis_proj * axis_dir;
        const float radial_dist_sq = math::length_squared(radial);

        const bool inside_axially = axis_proj >= -cylinder.half_height && axis_proj <= cylinder.half_height;
        const bool inside_radially = radial_dist_sq <= cylinder.radius * cylinder.radius;

        // Point lies inside the cylindrical volume.
        if (inside_axially && inside_radially)
        {
            return point;
        }

        // Base point on the axis used to construct the closest point.
        const math::vec3 axis_point = cylinder.center + clamped_proj * axis_dir;

        // Handle cap cases: outside axially but within the cap radius.
        if (!inside_axially && inside_radially)
        {
            return axis_point + radial;
        }

        // Otherwise we must project onto the lateral surface or the cap edge.
        if (radial_dist_sq == 0.0f)
        {
            // Degenerate case: the query lies exactly on the axis. Any radial direction is valid;
            // return the point on the axis (already clamped to the closest cap).
            return axis_point;
        }

        const math::vec3 radial_dir = radial / std::sqrt(radial_dist_sq);

        if (inside_axially)
        {
            // Lateral surface: preserve the original axial coordinate.
            const math::vec3 lateral_axis_point = cylinder.center + axis_proj * axis_dir;
            return lateral_axis_point + radial_dir * cylinder.radius;
        }

        // Cap edge: clamp axial coordinate and radial distance simultaneously.
        return axis_point + radial_dir * cylinder.radius;
    }

    double SquaredDistance(const Cylinder& cylinder, const math::vec3& point) noexcept
    {
        const math::vec3 closest = ClosestPoint(cylinder, point);
        return math::length_squared(point - closest);
    }
} // namespace engine::geometry
