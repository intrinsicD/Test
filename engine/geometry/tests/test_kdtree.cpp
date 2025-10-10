#include <gtest/gtest.h>

#include "engine/geometry/kdtree/kdtree.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/random.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

namespace geo = engine::geometry;
namespace math = engine::math;

namespace
{
    using Rng = geo::RandomEngine;

    math::vec3 random_point(Rng& rng)
    {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        return {dist(rng), dist(rng), dist(rng)};
    }

    std::vector<math::vec3> generate_points(std::size_t count, Rng& rng)
    {
        std::vector<math::vec3> points(count);
        std::generate(points.begin(), points.end(), [&]() { return random_point(rng); });
        return points;
    }

    std::vector<std::size_t> brute_force_aabb(const std::vector<math::vec3>& points, const geo::Aabb& region)
    {
        std::vector<std::size_t> hits;
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            if (geo::Contains(region, points[i]))
            {
                hits.push_back(i);
            }
        }
        std::sort(hits.begin(), hits.end());
        return hits;
    }

    std::vector<std::size_t> brute_force_radius(const std::vector<math::vec3>& points,
                                                const math::vec3& query,
                                                float radius)
    {
        const float radius_sq = radius * radius;
        std::vector<std::size_t> hits;
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            const math::vec3 diff = points[i] - query;
            if (math::length_squared(diff) <= radius_sq)
            {
                hits.push_back(i);
            }
        }
        std::sort(hits.begin(), hits.end());
        return hits;
    }

    std::vector<std::size_t> brute_force_knn(const std::vector<math::vec3>& points,
                                             const math::vec3& query,
                                             std::size_t k)
    {
        std::vector<std::pair<float, std::size_t>> distances;
        distances.reserve(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            const math::vec3 diff = points[i] - query;
            distances.emplace_back(math::length_squared(diff), i);
        }
        std::sort(distances.begin(), distances.end());
        k = std::min(k, distances.size());
        std::vector<std::size_t> indices(k);
        for (std::size_t i = 0; i < k; ++i)
        {
            indices[i] = distances[i].second;
        }
        std::sort(indices.begin(), indices.end());
        return indices;
    }
}

TEST(KdTree, QueryAabbMatchesBruteForce)
{
    Rng rng(42);
    const auto pts = generate_points(256, rng);

    geo::PropertySet elements;
    auto position_property = elements.add<math::vec3>("e:position", {});
    position_property.vector() = pts;

    geo::KdTree tree;
    ASSERT_TRUE(tree.build(position_property, 16, 24));
    ASSERT_TRUE(tree.validate_structure());

    std::vector<std::size_t> actual;
    for (int i = 0; i < 32; ++i)
    {
        geo::Aabb region;
        const math::vec3 min_corner = random_point(rng) - math::vec3{0.25f};
        const math::vec3 max_corner = min_corner + math::vec3{0.5f};
        region.min = min_corner;
        region.max = max_corner;

        auto expected = brute_force_aabb(pts, region);
        tree.query(region, actual);
        std::sort(actual.begin(), actual.end());
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t idx = 0; idx < expected.size(); ++idx)
        {
            EXPECT_EQ(actual[idx], expected[idx]);
        }
    }
}

TEST(KdTree, QueryRadiusMatchesBruteForce)
{
    Rng rng(99);
    const auto pts = generate_points(512, rng);

    geo::PropertySet elements;
    auto position_property = elements.add<math::vec3>("e:position", {});
    position_property.vector() = pts;

    geo::KdTree tree;
    ASSERT_TRUE(tree.build(position_property, 12, 32));
    ASSERT_TRUE(tree.validate_structure());

    std::uniform_real_distribution<float> radius_dist(0.05f, 0.35f);

    std::vector<std::size_t> actual;
    for (int i = 0; i < 32; ++i)
    {
        const math::vec3 query = random_point(rng);
        const float radius = radius_dist(rng);
        auto expected = brute_force_radius(pts, query, radius);
        tree.query_radius(query, radius, actual);
        std::sort(actual.begin(), actual.end());
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t idx = 0; idx < expected.size(); ++idx)
        {
            EXPECT_EQ(actual[idx], expected[idx]);
        }
    }
}

TEST(KdTree, QueryKnnMatchesBruteForce)
{
    Rng rng(7);
    const auto pts = generate_points(600, rng);

    geo::PropertySet elements;
    auto position_property = elements.add<math::vec3>("e:position", {});
    position_property.vector() = pts;

    geo::KdTree tree;
    ASSERT_TRUE(tree.build(position_property, 10, 32));
    ASSERT_TRUE(tree.validate_structure());

    std::vector<std::size_t> actual;
    for (int i = 0; i < 32; ++i)
    {
        const math::vec3 query = random_point(rng);
        const std::size_t k = static_cast<std::size_t>(3 + (i % 8));
        auto expected = brute_force_knn(pts, query, k);
        tree.query_knn(query, k, actual);
        std::sort(actual.begin(), actual.end());
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t idx = 0; idx < expected.size(); ++idx)
        {
            EXPECT_EQ(actual[idx], expected[idx]);
        }
    }
}

TEST(KdTree, QueryNearestMatchesBruteForce)
{
    Rng rng(1234);
    const auto pts = generate_points(450, rng);

    geo::PropertySet elements;
    auto position_property = elements.add<math::vec3>("e:position", {});
    position_property.vector() = pts;

    geo::KdTree tree;
    ASSERT_TRUE(tree.build(position_property, 8, 32));
    ASSERT_TRUE(tree.validate_structure());

    std::vector<std::pair<float, std::size_t>> distances;

    for (int i = 0; i < 32; ++i)
    {
        const math::vec3 query = random_point(rng);
        distances.clear();
        for (std::size_t pi = 0; pi < pts.size(); ++pi)
        {
            const math::vec3 diff = pts[pi] - query;
            distances.emplace_back(math::length_squared(diff), pi);
        }
        std::sort(distances.begin(), distances.end());
        const std::size_t expected = distances.front().second;

        std::size_t actual = std::numeric_limits<std::size_t>::max();
        tree.query_nearest(query, actual);
        EXPECT_EQ(actual, expected);
    }
}
