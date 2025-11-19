#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::tools::imgui
{
    struct PanelRenderContext;
}

namespace engine::tools::editor
{
    /// Dear ImGui panel that tracks runtime performance metrics and profiler summaries.
    class PerformanceMetricsPanel
    {
    public:
        struct FrameSample
        {
            double frame_ms{0.0};
            double average_ms{0.0};
            double max_ms{0.0};
        };

        struct StageTimingRow
        {
            std::string name{};
            double last_ms{0.0};
            double average_ms{0.0};
            double max_ms{0.0};
        };

        struct ProfilerEntryRow
        {
            std::string name{};
            double total_ms{0.0};
            double average_ms{0.0};
            double min_ms{0.0};
            double max_ms{0.0};
            std::uint64_t call_count{0};
        };

        struct BenchmarkEntry
        {
            std::string label{};
            double baseline_ms{0.0};
            double current_ms{0.0};
        };

        struct GpuPassTimingRow
        {
            std::string name{};
            std::string queue{};
            double duration_ms{0.0};
            std::uint64_t timestamp_begin_ns{0};
            std::uint64_t timestamp_end_ns{0};
        };

        void set_history_capacity(std::size_t capacity) noexcept;
        void push_frame_sample(const FrameSample& sample);
        void set_stage_timings(std::vector<StageTimingRow> rows);
        void set_profiler_entries(std::vector<ProfilerEntryRow> rows);
        void set_benchmark_entries(std::vector<BenchmarkEntry> entries);
        void set_gpu_pass_timings(std::vector<GpuPassTimingRow> rows);

        [[nodiscard]] const std::vector<float>& frame_history() const noexcept;
        [[nodiscard]] bool has_frame_sample() const noexcept;
        [[nodiscard]] const FrameSample& latest_sample() const noexcept;
        [[nodiscard]] const std::vector<GpuPassTimingRow>& gpu_pass_timings() const noexcept;

        void render(const imgui::PanelRenderContext& context);

    private:
        void trim_history();
        [[nodiscard]] float history_ceiling() const noexcept;

        FrameSample latest_sample_{};
        bool has_sample_{false};
        std::vector<float> frame_history_{};
        std::size_t history_capacity_{240};
        std::vector<StageTimingRow> stage_timings_{};
        std::vector<ProfilerEntryRow> profiler_entries_{};
        std::vector<BenchmarkEntry> benchmark_entries_{};
        std::vector<GpuPassTimingRow> gpu_pass_timings_{};
    };
}
