#include "engine/animation/benchmarking/telemetry.hpp"

#include <gtest/gtest.h>

namespace animation = engine::animation;
namespace benchmarking = animation::benchmarking;

TEST(AnimationBenchmarkTelemetry, AggregatesCategories)
{
    const benchmarking::DispatchTelemetry dispatches[] = {
        {"a", "animation.sample", "cpu", 0.5},
        {"b", "animation.sample", "gpu", 0.75},
        {"c", "animation.control", "cpu", 1.0},
        {"d", "", "gpu", 0.25},
    };

    const auto totals = benchmarking::aggregate_category_totals(dispatches);

    ASSERT_EQ(totals.size(), 3U);
    EXPECT_EQ(totals[0].label, "animation.control");
    EXPECT_DOUBLE_EQ(totals[0].duration_ms, 1.0);
    EXPECT_EQ(totals[1].label, "animation.sample");
    EXPECT_DOUBLE_EQ(totals[1].duration_ms, 1.25);
    EXPECT_EQ(totals[2].label, "uncategorised");
    EXPECT_DOUBLE_EQ(totals[2].duration_ms, 0.25);
}

TEST(AnimationBenchmarkTelemetry, AggregatesQueues)
{
    const benchmarking::DispatchTelemetry dispatches[] = {
        {"a", "animation.sample", "cpu", 0.5},
        {"b", "animation.sample", "gpu", 0.75},
        {"c", "animation.control", "cpu", 1.0},
        {"d", "", "", 0.25},
    };

    const auto totals = benchmarking::aggregate_queue_totals(dispatches);

    ASSERT_EQ(totals.size(), 3U);
    EXPECT_EQ(totals[0].label, "cpu");
    EXPECT_DOUBLE_EQ(totals[0].duration_ms, 1.5);
    EXPECT_EQ(totals[1].label, "gpu");
    EXPECT_DOUBLE_EQ(totals[1].duration_ms, 0.75);
    EXPECT_EQ(totals[2].label, "unspecified");
    EXPECT_DOUBLE_EQ(totals[2].duration_ms, 0.25);
}

TEST(AnimationBenchmarkTelemetry, Canonicalisation)
{
    EXPECT_EQ(benchmarking::canonical_category(""), "uncategorised");
    EXPECT_EQ(benchmarking::canonical_category("animation.sample"), "animation.sample");
    EXPECT_EQ(benchmarking::canonical_queue(""), "unspecified");
    EXPECT_EQ(benchmarking::canonical_queue("cpu"), "cpu");
}