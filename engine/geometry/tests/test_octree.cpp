#include <gtest/gtest.h>

#include "engine/geometry/octree/octree.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/shapes.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

namespace geo = engine::geometry;
namespace math = engine::math;

namespace
{
    using Rng = geo::RandomEngine;

    template <typename Query>
    void report_mismatch(const Query&,
                         const std::vector<geo::Aabb>&,
                         const std::vector<std::size_t>&,
                         const std::vector<std::size_t>&)
    {
    }

    std::vector<geo::Aabb> generate_random_aabbs(std::size_t count, Rng& rng)
    {
        std::vector<geo::Aabb> boxes(count);
        for (auto& box : boxes)
        {
            geo::Random(box, rng);
        }
        return boxes;
    }

    template <typename Query>
    std::vector<std::size_t> brute_force_intersection(const std::vector<geo::Aabb>& boxes, const Query& query)
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

    std::array<geo::Octree::SplitPolicy, 6> test_policies()
    {
        std::array<geo::Octree::SplitPolicy, 6> policies{};
        std::size_t idx = 0;
        for (auto split_point : {
                 geo::Octree::SplitPoint::Center,
                 geo::Octree::SplitPoint::Mean,
                 geo::Octree::SplitPoint::Median })
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

    template <typename QueryGenerator>
    void run_query_test(std::size_t num_boxes,
                        unsigned seed,
                        std::size_t query_count,
                        QueryGenerator&& generator,
                        std::size_t max_elements = 8,
                        std::size_t max_depth = 12)
    {
        Rng rng(seed);
        const auto boxes = generate_random_aabbs(num_boxes, rng);

        geo::PropertySet elements;
        auto aabb_property = elements.add<geo::Aabb>("e:aabb", {});
        aabb_property.vector() = boxes;

        const auto policies = test_policies();

        for (const auto& policy : policies)
        {
            geo::Octree tree;
            ASSERT_TRUE(tree.build(aabb_property, policy, max_elements, max_depth));
            ASSERT_TRUE(tree.validate_structure());

            for (std::size_t qi = 0; qi < query_count; ++qi)
            {
                const auto query = generator(rng);
                using QueryType = std::decay_t<decltype(query)>;
                const auto expected = brute_force_intersection(boxes, query);

                std::vector<std::size_t> actual;
                tree.query(query, actual);
                std::sort(actual.begin(), actual.end());
                if (actual.size() != expected.size())
                {
                    report_mismatch<QueryType>(query, boxes, expected, actual);
                }
                ASSERT_EQ(actual.size(), expected.size());
                for (std::size_t i = 0; i < expected.size(); ++i)
                {
                    if (actual[i] != expected[i])
                    {
                        report_mismatch<QueryType>(query, boxes, expected, actual);
                    }
                    EXPECT_EQ(actual[i], expected[i]);
                }
            }
        }
    }
}

TEST(Octree, QueryAabbMatchesBruteForce)
{
    run_query_test(200, 1337, 25, [](Rng& rng) {
        geo::Aabb query{};
        geo::Random(query, rng);
        const math::vec3 padding{0.1f};
        query.min -= padding;
        query.max += padding;
        return query;
    });
}

TEST(Octree, QuerySphereMatchesBruteForce)
{
    run_query_test(160, 2024, 25, [](Rng& rng) {
        geo::Sphere query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryRayMatchesBruteForce)
{
    run_query_test(180, 42, 20, [](Rng& rng) {
        geo::Ray query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryCylinderMatchesBruteForce)
{
    run_query_test(130, 1234, 20, [](Rng& rng) {
        geo::Cylinder query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryEllipsoidMatchesBruteForce)
{
    run_query_test(140, 2025, 20, [](Rng& rng) {
        geo::Ellipsoid query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryObbMatchesBruteForce)
{
    run_query_test(150, 31415, 20, [](Rng& rng) {
        geo::Obb query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryTriangleMatchesBruteForce)
{
    run_query_test(150, 2718, 20, [](Rng& rng) {
        geo::Triangle query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QuerySegmentMatchesBruteForce)
{
    run_query_test(150, 8080, 20, [](Rng& rng) {
        geo::Segment query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryLineMatchesBruteForce)
{
    run_query_test(150, 4242, 20, [](Rng& rng) {
        geo::Line query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryPlaneMatchesBruteForce)
{
    run_query_test(150, 5151, 20, [](Rng& rng) {
        geo::Plane query{};
        geo::Random(query, rng);
        return query;
    });
}

TEST(Octree, QueryKnnMatchesBruteForce)
{
    Rng rng(7);
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
    Rng rng(99);
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
