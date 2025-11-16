#include "engine/tools/editor/telemetry_visualization_panel.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <imgui.h>

#include "engine/tools/profiling/profiler.hpp"

namespace engine::tools::editor
{
    namespace
    {
        constexpr float kPlotHeight = 80.0F;

        [[nodiscard]] ImVec4 alert_color(TelemetryVisualizationPanel::AlertLevel level) noexcept
        {
            switch (level)
            {
            case TelemetryVisualizationPanel::AlertLevel::Warning:
                return ImVec4(1.0F, 0.8F, 0.2F, 1.0F);
            case TelemetryVisualizationPanel::AlertLevel::Critical:
                return ImVec4(1.0F, 0.2F, 0.2F, 1.0F);
            case TelemetryVisualizationPanel::AlertLevel::None:
            default:
                return ImVec4(0.6F, 0.8F, 0.6F, 1.0F);
            }
        }

        [[nodiscard]] std::string normalise_identifier(const TelemetryVisualizationPanel::SeriesSample& sample)
        {
            if (!sample.identifier.empty())
            {
                return sample.identifier;
            }

            if (!sample.label.empty())
            {
                return sample.label;
            }

            return "telemetry.series";
        }

        void render_alert_label(TelemetryVisualizationPanel::AlertLevel level)
        {
            const char* label = TelemetryVisualizationPanel::alert_label(level);
            if (!label)
            {
                return;
            }

            ImGui::TextColored(alert_color(level), "%s", label);
        }

        [[nodiscard]] ImVec2 history_plot_bounds(const std::vector<float>& history) noexcept
        {
            if (history.empty())
            {
                return ImVec2(0.0F, 1.0F);
            }

            auto [min_it, max_it] = std::minmax_element(history.begin(), history.end());
            float min_value = *min_it;
            float max_value = *max_it;
            if (min_value == max_value)
            {
                min_value -= 1.0F;
                max_value += 1.0F;
            }

            return ImVec2(min_value, max_value);
        }
    } // namespace

    void TelemetryVisualizationPanel::set_history_capacity(std::size_t capacity) noexcept
    {
        history_capacity_ = std::max<std::size_t>(1, capacity);
        for (auto& state : series_)
        {
            trim_history(state.history);
        }
    }

    void TelemetryVisualizationPanel::update_series(std::vector<SeriesSample> samples)
    {
        std::unordered_map<std::string, SeriesState> existing;
        existing.reserve(series_.size());
        for (auto& state : series_)
        {
            existing.emplace(state.sample.identifier, std::move(state));
        }

        std::vector<SeriesState> next;
        next.reserve(samples.size());

        for (auto& sample : samples)
        {
            sample.identifier = normalise_identifier(sample);

            SeriesState state{};
            if (auto it = existing.find(sample.identifier); it != existing.end())
            {
                state = std::move(it->second);
            }

            state.sample = std::move(sample);
            state.history.push_back(static_cast<float>(state.sample.value));
            trim_history(state.history);
            state.alert = classify_alert(state.sample.value, state.sample);

            next.push_back(std::move(state));
        }

        series_ = std::move(next);
    }

    const std::vector<TelemetryVisualizationPanel::SeriesState>& TelemetryVisualizationPanel::series() const noexcept
    {
        return series_;
    }

    void TelemetryVisualizationPanel::render(const imgui::PanelRenderContext&)
    {
        PROFILE_SCOPE("TelemetryVisualizationPanel");

        if (ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }

        if (!ImGui::Begin("Telemetry Visualization"))
        {
            ImGui::End();
            return;
        }

        if (series_.empty())
        {
            ImGui::TextUnformatted("Telemetry stream idle — waiting for diagnostics updates.");
            ImGui::End();
            return;
        }

        for (std::size_t index = 0; index < series_.size(); ++index)
        {
            const auto& state = series_[index];
            const std::string& label = state.sample.label.empty() ? state.sample.identifier : state.sample.label;

            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            render_alert_label(state.alert);

            const std::string& unit = state.sample.unit;
            if (unit.empty())
            {
                ImGui::Text("Current: %.3f", state.sample.value);
            }
            else
            {
                ImGui::Text("Current: %.3f %s", state.sample.value, unit.c_str());
            }

            if (state.sample.warning_threshold || state.sample.critical_threshold)
            {
                if (state.sample.warning_threshold && state.sample.critical_threshold)
                {
                    ImGui::Text(
                        "Warning ≥ %.3f | Critical ≥ %.3f",
                        *state.sample.warning_threshold,
                        *state.sample.critical_threshold
                    );
                }
                else if (state.sample.warning_threshold)
                {
                    ImGui::Text("Warning ≥ %.3f", *state.sample.warning_threshold);
                }
                else if (state.sample.critical_threshold)
                {
                    ImGui::Text("Critical ≥ %.3f", *state.sample.critical_threshold);
                }
            }
            else
            {
                ImGui::TextUnformatted("Warning/Critical thresholds not configured.");
            }

            if (!state.history.empty())
            {
                const ImVec2 bounds = history_plot_bounds(state.history);
                std::string plot_identifier = "##telemetry_plot_" + state.sample.identifier;
                ImGui::PlotLines(
                    plot_identifier.c_str(),
                    state.history.data(),
                    static_cast<int>(state.history.size()),
                    0,
                    nullptr,
                    bounds.x,
                    bounds.y,
                    ImVec2(0.0F, kPlotHeight)
                );
            }
            else
            {
                ImGui::TextUnformatted("No historical samples available.");
            }

            if (index + 1 < series_.size())
            {
                ImGui::Separator();
            }
        }

        ImGui::End();
    }

    void TelemetryVisualizationPanel::trim_history(std::vector<float>& history) const noexcept
    {
        if (history.size() <= history_capacity_)
        {
            return;
        }

        const auto remove_count = history.size() - history_capacity_;
        history.erase(history.begin(), history.begin() + static_cast<std::ptrdiff_t>(remove_count));
    }

    TelemetryVisualizationPanel::AlertLevel TelemetryVisualizationPanel::classify_alert(
        double value,
        const SeriesSample& sample
    ) noexcept
    {
        if (sample.critical_threshold && value >= *sample.critical_threshold)
        {
            return AlertLevel::Critical;
        }

        if (sample.warning_threshold && value >= *sample.warning_threshold)
        {
            return AlertLevel::Warning;
        }

        return AlertLevel::None;
    }

    const char* TelemetryVisualizationPanel::alert_label(AlertLevel level) noexcept
    {
        switch (level)
        {
        case AlertLevel::Warning:
            return "Warning";
        case AlertLevel::Critical:
            return "Critical";
        case AlertLevel::None:
        default:
            return "Nominal";
        }
    }
}

