#include <gtest/gtest.h>

#include <array>
#include <span>

#include "engine/animation/benchmarking/statistics.hpp"

namespace
{
    using engine::animation::benchmarking::FrameTimingSummary;
    using engine::animation::benchmarking::compute_frame_timing_summary;
}

TEST(AnimationBenchmarkStatistics, HandlesEmptySamples)
{
    const FrameTimingSummary summary = compute_frame_timing_summary({});
    EXPECT_EQ(summary.samples, 0U);
    EXPECT_DOUBLE_EQ(summary.total_ms, 0.0);
    EXPECT_DOUBLE_EQ(summary.mean_ms, 0.0);
    EXPECT_DOUBLE_EQ(summary.min_ms, 0.0);
    EXPECT_DOUBLE_EQ(summary.max_ms, 0.0);
    EXPECT_DOUBLE_EQ(summary.stddev_ms, 0.0);
}

TEST(AnimationBenchmarkStatistics, ComputesAggregateValues)
{
    constexpr std::array<double, 4> samples{1.0, 2.0, 3.0, 4.0};
    const FrameTimingSummary summary = compute_frame_timing_summary(std::span<const double>(samples));

    EXPECT_EQ(summary.samples, samples.size());
    EXPECT_DOUBLE_EQ(summary.total_ms, 10.0);
    EXPECT_DOUBLE_EQ(summary.min_ms, 1.0);
    EXPECT_DOUBLE_EQ(summary.max_ms, 4.0);
    EXPECT_DOUBLE_EQ(summary.mean_ms, 2.5);
    EXPECT_NEAR(summary.stddev_ms, 1.11803398875, 1e-9);
}

