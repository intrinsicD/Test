#include <gtest/gtest.h>

#include "engine/tools/editor/telemetry_visualization_panel.hpp"

namespace
{
    using engine::tools::editor::TelemetryVisualizationPanel;
}

TEST(TelemetryVisualizationPanel, TrimsHistoryToCapacity)
{
    TelemetryVisualizationPanel panel{};
    panel.set_history_capacity(2);

    TelemetryVisualizationPanel::SeriesSample sample{};
    sample.identifier = "streaming.pending";
    sample.label = "Streaming Pending";

    sample.value = 1.0;
    panel.update_series({sample});

    sample.value = 2.0;
    panel.update_series({sample});

    sample.value = 3.0;
    panel.update_series({sample});

    ASSERT_EQ(panel.series().size(), 1U);
    const auto& history = panel.series().front().history;
    ASSERT_EQ(history.size(), 2U);
    EXPECT_FLOAT_EQ(history[0], 2.0F);
    EXPECT_FLOAT_EQ(history[1], 3.0F);
}

TEST(TelemetryVisualizationPanel, ClassifiesAlertLevels)
{
    TelemetryVisualizationPanel panel{};
    panel.set_history_capacity(4);

    TelemetryVisualizationPanel::SeriesSample sample{};
    sample.identifier = "telemetry.series";
    sample.label = "Frame Budget";
    sample.warning_threshold = 10.0;
    sample.critical_threshold = 20.0;

    sample.value = 25.0;
    panel.update_series({sample});
    ASSERT_EQ(panel.series().front().alert, TelemetryVisualizationPanel::AlertLevel::Critical);

    sample.value = 12.0;
    panel.update_series({sample});
    ASSERT_EQ(panel.series().front().alert, TelemetryVisualizationPanel::AlertLevel::Warning);

    sample.value = 4.0;
    panel.update_series({sample});
    ASSERT_EQ(panel.series().front().alert, TelemetryVisualizationPanel::AlertLevel::None);
}

