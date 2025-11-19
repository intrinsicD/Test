#include <gtest/gtest.h>

#include "engine/tools/editor/performance_metrics_panel.hpp"

namespace
{
    using engine::tools::editor::PerformanceMetricsPanel;
}

TEST(PerformanceMetricsPanel, TrimsHistoryToCapacity)
{
    PerformanceMetricsPanel panel{};
    panel.set_history_capacity(2);

    panel.push_frame_sample(PerformanceMetricsPanel::FrameSample{1.0, 1.0, 1.0});
    panel.push_frame_sample(PerformanceMetricsPanel::FrameSample{2.0, 2.0, 2.0});
    panel.push_frame_sample(PerformanceMetricsPanel::FrameSample{3.0, 3.0, 3.0});

    const auto& history = panel.frame_history();
    ASSERT_EQ(history.size(), 2U);
    EXPECT_FLOAT_EQ(history[0], 2.0F);
    EXPECT_FLOAT_EQ(history[1], 3.0F);
}

TEST(PerformanceMetricsPanel, HistoryCapacityClampsToOne)
{
    PerformanceMetricsPanel panel{};
    panel.set_history_capacity(0);

    panel.push_frame_sample(PerformanceMetricsPanel::FrameSample{4.0, 4.0, 4.0});
    panel.push_frame_sample(PerformanceMetricsPanel::FrameSample{5.0, 5.0, 5.0});

    const auto& history = panel.frame_history();
    ASSERT_EQ(history.size(), 1U);
    EXPECT_FLOAT_EQ(history[0], 5.0F);
}

TEST(PerformanceMetricsPanel, StoresLatestSample)
{
    PerformanceMetricsPanel panel{};

    panel.push_frame_sample(PerformanceMetricsPanel::FrameSample{6.0, 7.0, 9.0});

    ASSERT_TRUE(panel.has_frame_sample());
    const auto& latest = panel.latest_sample();
    EXPECT_DOUBLE_EQ(latest.frame_ms, 6.0);
    EXPECT_DOUBLE_EQ(latest.average_ms, 7.0);
    EXPECT_DOUBLE_EQ(latest.max_ms, 9.0);
}

TEST(PerformanceMetricsPanel, StoresGpuPassRows)
{
    PerformanceMetricsPanel panel{};
    std::vector<PerformanceMetricsPanel::GpuPassTimingRow> rows;
    rows.emplace_back(PerformanceMetricsPanel::GpuPassTimingRow{
        .name = "ForwardGeometry",
        .queue = "Graphics",
        .duration_ms = 0.5,
        .timestamp_begin_ns = 10,
        .timestamp_end_ns = 20,
    });

    panel.set_gpu_pass_timings(rows);
    const auto& stored = panel.gpu_pass_timings();
    ASSERT_EQ(stored.size(), 1U);
    EXPECT_EQ(stored[0].name, "ForwardGeometry");
    EXPECT_EQ(stored[0].queue, "Graphics");
    EXPECT_DOUBLE_EQ(stored[0].duration_ms, 0.5);
    EXPECT_EQ(stored[0].timestamp_begin_ns, 10U);
    EXPECT_EQ(stored[0].timestamp_end_ns, 20U);
}
