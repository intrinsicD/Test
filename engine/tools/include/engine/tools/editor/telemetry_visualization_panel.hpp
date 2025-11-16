#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace engine::tools::imgui
{
    struct PanelRenderContext;
}

namespace engine::tools::editor
{
    /// Dear ImGui panel that visualises runtime telemetry series and alert thresholds.
    class TelemetryVisualizationPanel
    {
    public:
        struct SeriesSample
        {
            std::string identifier{};
            std::string label{};
            double value{0.0};
            std::optional<double> warning_threshold{};
            std::optional<double> critical_threshold{};
            std::string unit{};
        };

        enum class AlertLevel
        {
            None,
            Warning,
            Critical,
        };

        struct SeriesState
        {
            SeriesSample sample{};
            std::vector<float> history{};
            AlertLevel alert{AlertLevel::None};
        };

        void set_history_capacity(std::size_t capacity) noexcept;
        void update_series(std::vector<SeriesSample> samples);

        [[nodiscard]] const std::vector<SeriesState>& series() const noexcept;

        void render(const imgui::PanelRenderContext& context);

        static const char* alert_label(AlertLevel level) noexcept;

    private:
        void trim_history(std::vector<float>& history) const noexcept;
        static AlertLevel classify_alert(double value, const SeriesSample& sample) noexcept;
        std::vector<SeriesState> series_{};
        std::size_t history_capacity_{180};
    };
}

