#include "engine/tools/editor/performance_metrics_panel.hpp"

#include <algorithm>
#include <cinttypes>
#include <cstddef>

#include <imgui.h>

#include "engine/tools/profiling/profiler.hpp"

namespace engine::tools::editor
{
    namespace
    {
        constexpr float kDefaultPlotHeight = 120.0F;
        constexpr float kMinimumPlotCeiling = 5.0F;
    }

    void PerformanceMetricsPanel::set_history_capacity(std::size_t capacity) noexcept
    {
        history_capacity_ = std::max<std::size_t>(1, capacity);
        trim_history();
    }

    void PerformanceMetricsPanel::push_frame_sample(const FrameSample& sample)
    {
        latest_sample_ = sample;
        has_sample_ = true;

        frame_history_.push_back(static_cast<float>(sample.frame_ms));
        trim_history();
    }

    void PerformanceMetricsPanel::set_stage_timings(std::vector<StageTimingRow> rows)
    {
        stage_timings_ = std::move(rows);
    }

    void PerformanceMetricsPanel::set_profiler_entries(std::vector<ProfilerEntryRow> rows)
    {
        profiler_entries_ = std::move(rows);
    }

    void PerformanceMetricsPanel::set_benchmark_entries(std::vector<BenchmarkEntry> entries)
    {
        benchmark_entries_ = std::move(entries);
    }

    const std::vector<float>& PerformanceMetricsPanel::frame_history() const noexcept
    {
        return frame_history_;
    }

    bool PerformanceMetricsPanel::has_frame_sample() const noexcept
    {
        return has_sample_;
    }

    const PerformanceMetricsPanel::FrameSample& PerformanceMetricsPanel::latest_sample() const noexcept
    {
        return latest_sample_;
    }

    void PerformanceMetricsPanel::render(const imgui::PanelRenderContext&)
    {
        PROFILE_SCOPE("PerformanceMetricsPanel");

        if (ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }

        if (!ImGui::Begin("Performance Metrics"))
        {
            ImGui::End();
            return;
        }

        if (has_sample_)
        {
            ImGui::Text("Latest frame: %.3f ms (avg %.3f ms, max %.3f ms)",
                        latest_sample_.frame_ms,
                        latest_sample_.average_ms,
                        latest_sample_.max_ms);
        }
        else
        {
            ImGui::TextUnformatted("Frame timings unavailable.");
        }

        if (!frame_history_.empty())
        {
            const float y_max = history_ceiling();
            ImGui::PlotLines("Frame Time History",
                             frame_history_.data(),
                             static_cast<int>(frame_history_.size()),
                             0,
                             nullptr,
                             0.0F,
                             y_max,
                             ImVec2(0.0F, kDefaultPlotHeight));
        }
        else
        {
            ImGui::TextUnformatted("Waiting for runtime frame samples...");
        }

        ImGui::Separator();
        if (!stage_timings_.empty())
        {
            if (ImGui::BeginTable("StageTimings", 4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                                      | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Stage");
                ImGui::TableSetupColumn("Last (ms)");
                ImGui::TableSetupColumn("Average (ms)");
                ImGui::TableSetupColumn("Max (ms)");
                ImGui::TableHeadersRow();

                for (const auto& row : stage_timings_)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(row.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", row.last_ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", row.average_ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", row.max_ms);
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextUnformatted("Stage timing telemetry not yet captured.");
        }

        ImGui::Separator();
        if (!profiler_entries_.empty())
        {
            if (ImGui::BeginTable("ProfilerEntries", 5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                                      | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable))
            {
                ImGui::TableSetupColumn("Zone");
                ImGui::TableSetupColumn("Calls");
                ImGui::TableSetupColumn("Total (ms)");
                ImGui::TableSetupColumn("Average (ms)");
                ImGui::TableSetupColumn("Min / Max (ms)");
                ImGui::TableHeadersRow();

                for (const auto& entry : profiler_entries_)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(entry.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%" PRIu64, entry.call_count);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", entry.total_ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", entry.average_ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f / %.3f", entry.min_ms, entry.max_ms);
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextUnformatted("Profiler data unavailable. Ensure PROFILE_SCOPE() emits zones.");
        }

        ImGui::Separator();
        if (!benchmark_entries_.empty())
        {
            if (ImGui::BeginTable("BenchmarkSummary", 4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                                      | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Benchmark");
                ImGui::TableSetupColumn("Baseline (ms)");
                ImGui::TableSetupColumn("Current (ms)");
                ImGui::TableSetupColumn("Delta (ms)");
                ImGui::TableHeadersRow();

                for (const auto& entry : benchmark_entries_)
                {
                    const double delta = entry.current_ms - entry.baseline_ms;

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(entry.label.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", entry.baseline_ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", entry.current_ms);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", delta);
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextUnformatted("Benchmark history unavailable. Run PM-510 captures to populate deltas.");
        }

        ImGui::End();
    }

    void PerformanceMetricsPanel::trim_history()
    {
        if (frame_history_.size() <= history_capacity_)
        {
            return;
        }

        const auto remove_count = frame_history_.size() - history_capacity_;
        frame_history_.erase(frame_history_.begin(), frame_history_.begin() + static_cast<std::ptrdiff_t>(remove_count));
    }

    float PerformanceMetricsPanel::history_ceiling() const noexcept
    {
        float max_value = 0.0F;
        for (float value : frame_history_)
        {
            max_value = std::max(max_value, value);
        }

        if (max_value <= 0.0F)
        {
            return kMinimumPlotCeiling;
        }

        return std::max(kMinimumPlotCeiling, max_value * 1.25F);
    }
}
