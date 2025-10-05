#include <gtest/gtest.h>

#include "engine/geometry/octree/octree.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <vector>

namespace geo = engine::geometry;
namespace math = engine::math;

namespace
{
    std::vector<geo::Aabb> generate_random_aabbs(std::size_t count, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> center_dist(-10.0f, 10.0f);
        std::uniform_real_distribution<float> extent_dist(0.05f, 1.5f);

        std::vector<geo::Aabb> boxes;
        boxes.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            math::vec3 center(center_dist(rng), center_dist(rng), center_dist(rng));
            math::vec3 half_extent(extent_dist(rng), extent_dist(rng), extent_dist(rng));

            geo::Aabb box;
            box.min = center - half_extent;
            box.max = center + half_extent;
            boxes.push_back(box);
        }
        return boxes;
    }

    geo::Sphere make_query_sphere(std::mt19937& rng)
    {
        std::uniform_real_distribution<float> center_dist(-12.0f, 12.0f);
        std::uniform_real_distribution<float> radius_dist(0.05f, 5.0f);
        return geo::Sphere{
            .center = math::vec3(center_dist(rng), center_dist(rng), center_dist(rng)),
            .radius = radius_dist(rng)
        };
    }

    geo::Aabb make_query_aabb(std::mt19937& rng)
    {
        std::uniform_real_distribution<float> center_dist(-12.0f, 12.0f);
        std::uniform_real_distribution<float> extent_dist(0.05f, 5.0f);
        math::vec3 center(center_dist(rng), center_dist(rng), center_dist(rng));
        math::vec3 half_extent(extent_dist(rng), extent_dist(rng), extent_dist(rng));
        return geo::Aabb{.min = center - half_extent, .max = center + half_extent};
    }

    std::vector<std::pair<float, std::size_t>> brute_force_distances(const std::vector<geo::Aabb>& boxes,
                                                                     const math::vec3& point)
    {
        std::vector<std::pair<float, std::size_t>> distances;
        distances.reserve(boxes.size());
        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            const float d2 = static_cast<float>(geo::SquaredDistance(boxes[i], point));
            distances.emplace_back(d2, i);
        }
        std::sort(distances.begin(), distances.end());
        return distances;
    }

    std::vector<std::size_t> brute_force_intersection(const std::vector<geo::Aabb>& boxes, const geo::Aabb& query)
    {
        std::vector<std::size_t> hits;
        hits.reserve(boxes.size());
        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            if (geo::Intersects(boxes[i], query))
            {
                hits.push_back(i);
            }
        }
        std::sort(hits.begin(), hits.end());
        return hits;
    }

    std::vector<std::size_t> brute_force_intersection(const std::vector<geo::Aabb>& boxes, const geo::Sphere& query)
    {
        std::vector<std::size_t> hits;
        hits.reserve(boxes.size());
        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            if (geo::Intersects(boxes[i], query))
            {
                hits.push_back(i);
            }
        }
        std::sort(hits.begin(), hits.end());
        return hits;
    }

    std::array<geo::Octree::SplitPolicy, 6> test_policies()
    {
        std::array<geo::Octree::SplitPolicy, 6> policies{};
        std::size_t idx = 0;
        for (auto split_point : {
                 geo::Octree::SplitPoint::Center,
                 geo::Octree::SplitPoint::Mean,
                 geo::Octree::SplitPoint::Median
             })
        {
            for (bool tight : {false, true})
            {
                geo::Octree::SplitPolicy policy;
                policy.split_point = split_point;
                policy.tight_children = tight;
                policy.epsilon = tight ? 1e-4f : 0.0f;
                policies[idx++] = policy;
            }
        }
        return policies;
    }
}

TEST(Octree, QueryAabbMatchesBruteForce)
{
    std::mt19937 rng(1337);
    const auto boxes = generate_random_aabbs(200, rng);

    geo::PropertySet elements;
    auto aabb_property = elements.add<geo::Aabb>("e:aabb", {});
    aabb_property.vector() = boxes;

    const auto policies = test_policies();

    for (const auto& policy : policies)
    {
        geo::Octree tree;
        ASSERT_TRUE(tree.build(aabb_property, policy, 8, 12));
        ASSERT_TRUE(tree.validate_structure());

        for (int qi = 0; qi < 25; ++qi)
        {
            geo::Aabb query = make_query_aabb(rng);

            const auto expected = brute_force_intersection(boxes, query);

            std::vector<std::size_t> actual;
            tree.query(query, actual);
            std::sort(actual.begin(), actual.end());
            ASSERT_EQ(actual.size(), expected.size());
            for (std::size_t i = 0; i < expected.size(); ++i)
            {
                EXPECT_EQ(actual[i], expected[i]);
            }
        }
    }
}

TEST(Octree, QuerySphereMatchesBruteForce)
{
    std::mt19937 rng(2024);
    const auto boxes = generate_random_aabbs(160, rng);

    geo::PropertySet elements;
    auto aabb_property = elements.add<geo::Aabb>("e:aabb", {});
    aabb_property.vector() = boxes;

    const auto policies = test_policies();

    for (const auto& policy : policies)
    {
        geo::Octree tree;
        ASSERT_TRUE(tree.build(aabb_property, policy, 8, 12));
        ASSERT_TRUE(tree.validate_structure());

        for (int qi = 0; qi < 25; ++qi)
        {
            geo::Sphere query = make_query_sphere(rng);

            const auto expected = brute_force_intersection(boxes, query);

            std::vector<std::size_t> actual;
            tree.query(query, actual);
            std::sort(actual.begin(), actual.end());
            ASSERT_EQ(actual.size(), expected.size());
            for (std::size_t i = 0; i < expected.size(); ++i)
            {
                EXPECT_EQ(actual[i], expected[i]);
            }
        }
    }
}

TEST(Octree, QueryKnnMatchesBruteForce)
{
    std::mt19937 rng(7);
    const auto boxes = generate_random_aabbs(150, rng);

    geo::PropertySet elements;
    auto aabb_property = elements.add<geo::Aabb>("e:aabb", {});
    aabb_property.vector() = boxes;

    const auto policies = test_policies();

    const std::array<std::size_t, 5> ks{0, 1, 3, 8, boxes.size() + 5};

    for (const auto& policy : policies)
    {
        geo::Octree tree;
        ASSERT_TRUE(tree.build(aabb_property, policy, 6, 12));
        ASSERT_TRUE(tree.validate_structure());

        std::uniform_real_distribution<float> point_dist(-15.0f, 15.0f);
        for (int qi = 0; qi < 20; ++qi)
        {
            math::vec3 query_point(point_dist(rng), point_dist(rng), point_dist(rng));

            const auto distances = brute_force_distances(boxes, query_point);

            for (std::size_t k : ks)
            {
                std::vector<std::size_t> actual;
                tree.query_knn(query_point, k, actual);

                const std::size_t expected_count = std::min<std::size_t>(k, distances.size());

                std::vector<std::size_t> expected;
                expected.reserve(expected_count);
                for (std::size_t i = 0; i < expected_count; ++i)
                {
                    expected.push_back(distances[i].second);
                }

                EXPECT_EQ(actual.size(), expected_count);
                for (std::size_t i = 0; i < actual.size(); ++i)
                {
                    EXPECT_EQ(actual[i], expected[i]);
                }
            }
        }
    }
}

TEST(Octree, QueryNearestMatchesBruteForce)
{
    std::mt19937 rng(99);
    const auto boxes = generate_random_aabbs(120, rng);

    geo::PropertySet elements;
    auto aabb_property = elements.add<geo::Aabb>("e:aabb", {});
    aabb_property.vector() = boxes;

    const auto policies = test_policies();

    for (const auto& policy : policies)
    {
        geo::Octree tree;
        ASSERT_TRUE(tree.build(aabb_property, policy, 6, 12));
        ASSERT_TRUE(tree.validate_structure());

        std::uniform_real_distribution<float> point_dist(-18.0f, 18.0f);
        for (int qi = 0; qi < 25; ++qi)
        {
            math::vec3 query_point(point_dist(rng), point_dist(rng), point_dist(rng));

            const auto distances = brute_force_distances(boxes, query_point);

            std::size_t nearest = std::numeric_limits<std::size_t>::max();
            tree.query_nearest(query_point, nearest);

            EXPECT_EQ(nearest, distances.front().second);
        }
    }
}
