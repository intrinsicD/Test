#pragma once

#include "engine/tools/api.hpp"

#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::tools::sandbox
{
    struct SandboxBenchmarkResult
    {
        bool success{false};
        std::string headline;
        std::string details;
    };

    struct DatasetDescriptor
    {
        std::string identifier;
        std::string label;
        std::string kind;
        std::vector<std::string> tags;
        std::map<std::string, double> statistics;
        std::map<std::string, double> metrics;
        std::string source_asset;
        std::string processed_asset;
    };

    struct OverlayDescriptor
    {
        std::string key;
        std::string label;
        bool default_enabled{false};
    };

    struct RenderingPresetDescriptor
    {
        std::string identifier;
        std::string label;
        std::pair<int, int> default_resolution{1920, 1080};
        std::vector<std::string> shading_modes;
        std::vector<OverlayDescriptor> overlays;
    };

    struct RuntimeSummary
    {
        std::string dataset_identifier;
        std::string scene_manifest;
        std::string scene_entry_point;
        std::string simulation_description;
        std::string camera_description;
        bool hot_reload_enabled{false};
    };

    struct TelemetrySeries
    {
        std::string name;
        std::vector<float> samples;
        float minimum{0.0F};
        float maximum{0.0F};
    };

    struct TelemetrySnapshot
    {
        float frames_per_second{0.0F};
        float cpu_frame_time_ms{0.0F};
        float gpu_frame_time_ms{0.0F};
        std::string status_message;
        std::vector<TelemetrySeries> series;
    };

    struct SandboxPreferences
    {
        std::string selected_dataset;
        std::string selected_preset;
        std::string shading_mode;
        std::unordered_map<std::string, bool> overlays;
        int benchmark_frames{600};
        float benchmark_timestep{1.0F / 60.0F};
    };

    struct ExperimentConfigurationSummary
    {
        std::vector<DatasetDescriptor> datasets;
        std::optional<std::string> selected_dataset;
        std::vector<RenderingPresetDescriptor> rendering_presets;
        RuntimeSummary runtime;
    };

    struct SandboxCallbacks
    {
        std::function<void(const std::string& dataset_id)> on_dataset_selected;
        std::function<void(const SandboxPreferences& preferences)> on_rendering_changed;
        std::function<SandboxBenchmarkResult(const SandboxPreferences& preferences)> on_run_benchmark;
    };

    class ENGINE_TOOLS_API ExperimentSandbox
    {
    public:
        ExperimentSandbox();

        void set_configuration(const ExperimentConfigurationSummary& summary);
        void update_telemetry(const TelemetrySnapshot& telemetry);

        void set_callbacks(SandboxCallbacks callbacks);

        void render();

        [[nodiscard]] const SandboxPreferences& preferences() const noexcept;
        void set_preferences(const SandboxPreferences& preferences);

        bool load_preferences(const std::filesystem::path& path);
        bool save_preferences(const std::filesystem::path& path) const;

        bool load_layout(const std::filesystem::path& path);
        bool save_layout(const std::filesystem::path& path) const;

        void apply_benchmark_result(SandboxBenchmarkResult result);
        [[nodiscard]] const std::optional<SandboxBenchmarkResult>& last_benchmark_result() const noexcept;

    private:
        void ensure_selection_defaults();
        void render_dataset_panel();
        void render_details_panel();
        void render_rendering_panel();
        void render_benchmark_panel();
        void render_telemetry_panel();
        void render_runtime_summary();

        [[nodiscard]] bool matches_dataset_filter(std::string_view text) const;
        void sync_overlay_preferences();

        ExperimentConfigurationSummary summary_{};
        TelemetrySnapshot telemetry_{};
        SandboxCallbacks callbacks_{};
        SandboxPreferences preferences_{};
        std::optional<SandboxBenchmarkResult> last_benchmark_result_{};

        std::unordered_map<std::string, std::size_t> dataset_lookup_{};
        std::unordered_map<std::string, std::size_t> preset_lookup_{};

        int selected_dataset_index_{-1};
        int selected_preset_index_{-1};

        std::array<char, 128> dataset_filter_buffer_{};
        std::string dataset_filter_{};
        std::string dataset_filter_lower_{};
    };
} // namespace engine::tools::sandbox

