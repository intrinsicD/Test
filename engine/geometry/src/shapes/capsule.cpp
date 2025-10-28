#include "engine/geometry/shapes/capsule.hpp"

#include "engine/math/math.hpp"
#include "engine/math/utils/utils.hpp"

#include <cmath>
#include <limits>
#include <numbers>

namespace engine::geometry
{
    namespace
    {
        [[nodiscard]] constexpr double four_thirds() noexcept
        {
            return 4.0 / 3.0;
        }

        [[nodiscard]] constexpr float epsilon() noexcept
        {
            return std::numeric_limits<float>::epsilon();
        }
    } // namespace

    math::vec3 Center(const Capsule& capsule) noexcept
    {
        return (capsule.point_a + capsule.point_b) * 0.5f;
    }

    math::vec3 Axis(const Capsule& capsule) noexcept
    {
        return capsule.point_b - capsule.point_a;
    }

    math::vec3 AxisDirection(const Capsule& capsule) noexcept
    {
        const math::vec3 axis = Axis(capsule);
        const float length = math::length(axis);
        if (length <= epsilon())
        {
            return math::vec3{0.0f, 0.0f, 0.0f};
        }
        return axis / length;
    }

    float Length(const Capsule& capsule) noexcept
    {
        return math::length(Axis(capsule));
    }

    double SurfaceArea(const Capsule& capsule) noexcept
    {
        const double radius = static_cast<double>(capsule.radius);
        const double height = static_cast<double>(Length(capsule));
        const double cylinder_area = 2.0 * std::numbers::pi_v<double> * radius * height;
        const double sphere_area = 4.0 * std::numbers::pi_v<double> * radius * radius;
        return cylinder_area + sphere_area;
    }

    double Volume(const Capsule& capsule) noexcept
    {
        const double radius = static_cast<double>(capsule.radius);
        const double height = static_cast<double>(Length(capsule));
        const double cylinder_volume = std::numbers::pi_v<double> * radius * radius * height;
        const double sphere_volume = four_thirds() * std::numbers::pi_v<double> * radius * radius * radius;
        return cylinder_volume + sphere_volume;
    }

    math::vec3 ClosestPoint(const Capsule& capsule, const math::vec3& point) noexcept
    {
        const math::vec3 axis = Axis(capsule);
        const float axis_len_sq = math::length_squared(axis);
        if (axis_len_sq <= epsilon())
        {
            const math::vec3 offset = point - capsule.point_a;
            const float dist_sq = math::length_squared(offset);
            if (dist_sq <= capsule.radius * capsule.radius || dist_sq <= epsilon())
            {
                return point;
            }
            const float dist = math::utils::sqrt(dist_sq);
            return capsule.point_a + (capsule.radius / dist) * offset;
        }

        const float t = math::utils::clamp(math::dot(point - capsule.point_a, axis) / axis_len_sq, 0.0f, 1.0f);
        const math::vec3 closest_on_segment = capsule.point_a + t * axis;
        const math::vec3 to_point = point - closest_on_segment;
        const float dist_sq = math::length_squared(to_point);
        if (dist_sq <= capsule.radius * capsule.radius || dist_sq <= epsilon())
        {
            return point;
        }
        const float dist = math::utils::sqrt(dist_sq);
        return closest_on_segment + (capsule.radius / dist) * to_point;
    }

    double SquaredDistance(const Capsule& capsule, const math::vec3& point) noexcept
    {
        const math::vec3 closest = ClosestPoint(capsule, point);
        return math::length_squared(point - closest);
    }

} // namespace engine::geometry

