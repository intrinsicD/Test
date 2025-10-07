#include "engine/geometry/shapes.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/utils.hpp"
#include "engine/math/utils_rotation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <random>

namespace engine::geometry
{
    namespace
    {
        constexpr float kDefaultCenterRange = 10.0f;
        constexpr float kExtendedCenterRange = 12.0f;

        RandomEngine& default_engine() noexcept
        {
            thread_local RandomEngine engine{[]
            {
                std::random_device device;
                return RandomEngine{device()};
            }()};
            return engine;
        }

        float uniform(RandomEngine& rng, float min_value, float max_value) noexcept
        {
            std::uniform_real_distribution<float> dist(min_value, max_value);
            return dist(rng);
        }

        math::vec3 uniform_vec3(RandomEngine& rng, float min_value, float max_value) noexcept
        {
            return math::vec3{uniform(rng, min_value, max_value),
                               uniform(rng, min_value, max_value),
                               uniform(rng, min_value, max_value)};
        }

        math::vec3 random_unit_vector(RandomEngine& rng) noexcept
        {
            std::normal_distribution<float> normal_dist(0.0f, 1.0f);
            math::vec3 candidate{0.0f};
            do
            {
                candidate = math::vec3{normal_dist(rng), normal_dist(rng), normal_dist(rng)};
            } while (math::length_squared(candidate) <= std::numeric_limits<float>::epsilon());
            return math::normalize(candidate);
        }

        math::quat random_unit_quaternion(RandomEngine& rng) noexcept
        {
            const math::vec3 axis = random_unit_vector(rng);
            const float angle = uniform(rng, 0.0f, static_cast<float>(2.0 * std::numbers::pi));
            const float half_angle = 0.5f * angle;
            const float sin_half = std::sin(half_angle);
            return math::normalize(math::quat(std::cos(half_angle), axis * sin_half));
        }

        void ensure_non_degenerate(math::vec3& e0, math::vec3& e1, RandomEngine& rng) noexcept
        {
            constexpr float kMinArea = 1e-4f;
            int attempts = 0;
            while (math::length_squared(math::cross(e0, e1)) < kMinArea && attempts < 8)
            {
                e1 = uniform_vec3(rng, -2.0f, 2.0f);
                ++attempts;
            }
            if (math::length_squared(math::cross(e0, e1)) < kMinArea)
            {
                const math::vec3 axis = random_unit_vector(rng);
                e1 = math::cross(axis, e0);
            }
        }
    } // namespace

    void Random(Aabb& box, RandomEngine& rng) noexcept
    {
        const math::vec3 center = uniform_vec3(rng, -kDefaultCenterRange, kDefaultCenterRange);
        const math::vec3 half_extent = uniform_vec3(rng, 0.25f, 3.0f);
        box.min = center - half_extent;
        box.max = center + half_extent;
    }

    void Random(Aabb& box) noexcept
    {
        Random(box, default_engine());
    }

    void Random(Sphere& sphere, RandomEngine& rng) noexcept
    {
        sphere.center = uniform_vec3(rng, -kDefaultCenterRange, kDefaultCenterRange);
        sphere.radius = uniform(rng, 0.25f, 4.0f);
    }

    void Random(Sphere& sphere) noexcept
    {
        Random(sphere, default_engine());
    }

    void Random(Cylinder& cylinder, RandomEngine& rng) noexcept
    {
        cylinder.center = uniform_vec3(rng, -kDefaultCenterRange, kDefaultCenterRange);
        cylinder.axis = random_unit_vector(rng);
        cylinder.radius = uniform(rng, 0.2f, 3.0f);
        cylinder.half_height = uniform(rng, 0.4f, 4.0f);
    }

    void Random(Cylinder& cylinder) noexcept
    {
        Random(cylinder, default_engine());
    }

    void Random(Ellipsoid& ellipsoid, RandomEngine& rng) noexcept
    {
        ellipsoid.center = uniform_vec3(rng, -kDefaultCenterRange, kDefaultCenterRange);
        ellipsoid.radii = uniform_vec3(rng, 0.3f, 4.0f);
        ellipsoid.orientation = random_unit_quaternion(rng);
    }

    void Random(Ellipsoid& ellipsoid) noexcept
    {
        Random(ellipsoid, default_engine());
    }

    void Random(Line& line, RandomEngine& rng) noexcept
    {
        line.point = uniform_vec3(rng, -kExtendedCenterRange, kExtendedCenterRange);
        line.direction = random_unit_vector(rng);
    }

    void Random(Line& line) noexcept
    {
        Random(line, default_engine());
    }

    void Random(Obb& box, RandomEngine& rng) noexcept
    {
        box.center = uniform_vec3(rng, -kDefaultCenterRange, kDefaultCenterRange);
        box.half_sizes = uniform_vec3(rng, 0.3f, 4.0f);
        box.orientation = random_unit_quaternion(rng);
    }

    void Random(Obb& box) noexcept
    {
        Random(box, default_engine());
    }

    void Random(Plane& plane, RandomEngine& rng) noexcept
    {
        plane.normal = random_unit_vector(rng);
        plane.distance = uniform(rng, -kExtendedCenterRange, kExtendedCenterRange);
    }

    void Random(Plane& plane) noexcept
    {
        Random(plane, default_engine());
    }

    void Random(Ray& ray, RandomEngine& rng) noexcept
    {
        ray.origin = uniform_vec3(rng, -kExtendedCenterRange, kExtendedCenterRange);
        ray.direction = random_unit_vector(rng);
    }

    void Random(Ray& ray) noexcept
    {
        Random(ray, default_engine());
    }

    void Random(Segment& segment, RandomEngine& rng) noexcept
    {
        segment.start = uniform_vec3(rng, -kExtendedCenterRange, kExtendedCenterRange);
        const math::vec3 direction = random_unit_vector(rng);
        const float length = uniform(rng, 0.25f, 6.0f);
        segment.end = segment.start + direction * length;
    }

    void Random(Segment& segment) noexcept
    {
        Random(segment, default_engine());
    }

    void Random(Triangle& triangle, RandomEngine& rng) noexcept
    {
        triangle.a = uniform_vec3(rng, -kDefaultCenterRange, kDefaultCenterRange);
        math::vec3 edge0 = uniform_vec3(rng, -2.0f, 2.0f);
        math::vec3 edge1 = uniform_vec3(rng, -2.0f, 2.0f);
        ensure_non_degenerate(edge0, edge1, rng);
        triangle.b = triangle.a + edge0;
        triangle.c = triangle.a + edge1;
    }

    void Random(Triangle& triangle) noexcept
    {
        Random(triangle, default_engine());
    }
}
