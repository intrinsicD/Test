#pragma once

#include "engine/tools/api.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
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

    struct DatasetAssetDescriptor
    {
        std::string role;
        std::string path;
        std::string resolved_path;
        bool exists{false};
        bool verified{false};
        std::optional<std::uintmax_t> expected_size_bytes;
        std::optional<std::uintmax_t> actual_size_bytes;
        std::optional<std::string> expected_sha256;
        std::optional<std::string> actual_sha256;
        std::optional<std::string> message;
    };

    struct DatasetDescriptor
    {
        std::string identifier;
        std::string label;
        std::string kind;
        std::string schema_id;
        int schema_version{0};
        std::vector<std::string> tags;
        std::map<std::string, double> statistics;
        struct StatisticGroup
        {
            std::string name;
            std::vector<std::pair<std::string, double>> entries;
        };
        std::vector<StatisticGroup> statistic_groups;
        std::map<std::string, double> metrics;
        std::string source_generator;
        std::string source_asset;
        std::optional<std::string> source_asset_sha256;
        std::optional<std::uintmax_t> source_asset_size_bytes;
        std::string processed_asset;
        std::optional<std::string> processed_asset_sha256;
        std::optional<std::uintmax_t> processed_asset_size_bytes;
        std::string remeshing_mode;
        std::vector<std::pair<std::string, double>> remeshing_targets;
        std::vector<std::pair<std::string, std::string>> feature_preservation;
        std::vector<std::pair<std::string, std::string>> parameterization_properties;
        std::vector<DatasetAssetDescriptor> assets;
    };

    struct CaseStudyDescriptor
    {
        std::string identifier;
        std::string label;
        std::string description;
        std::vector<std::string> tags;
        std::string config_path;
        std::string config_absolute;
        std::optional<std::string> default_dataset;
        std::optional<std::string> default_rendering_preset;
        std::optional<std::string> default_runtime_profile;
        std::optional<std::string> default_shading_mode;
        std::optional<int> default_resolution_width;
        std::optional<int> default_resolution_height;
        std::map<std::string, bool> default_overlays;
        std::vector<std::string> benchmark_scenarios;
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

    struct AlgorithmVariantDescriptor
    {
        std::string identifier;
        std::string label;
        std::optional<std::string> description;
    };

    struct RuntimeHotReloadDescriptor
    {
        bool enabled{false};
        std::optional<double> watch_interval_seconds;
    };

    struct RuntimeSummary
    {
        std::string dataset_identifier;
        std::string scene_manifest;
        std::optional<std::string> scene_manifest_path;
        std::string scene_entry_point;
        std::string simulation_description;
        std::string camera_description;
        int schema_version{0};
        RuntimeHotReloadDescriptor hot_reload;
    };

    struct TelemetryOutputDescriptor
    {
        std::string kind;
        std::optional<std::string> path;
        std::optional<std::string> template_path;
    };

    struct TelemetryMetricDescriptor
    {
        std::string name;
        std::string statistic;
    };

    struct TelemetrySamplingDescriptor
    {
        int frame_interval{0};
        bool include_debug_overlays{false};
    };

    struct TelemetryConfigurationDescriptor
    {
        int schema_version{0};
        std::vector<TelemetryOutputDescriptor> outputs;
        std::vector<TelemetryMetricDescriptor> metrics;
        std::optional<TelemetrySamplingDescriptor> sampling;
    };

    struct BenchmarkCommandDescriptor
    {
        std::vector<std::string> command;
        std::string output;
    };

    struct BenchmarkMetricThresholdDescriptor
    {
        std::string mode;
        double limit{0.0};
    };

    struct BenchmarkMetricDescriptor
    {
        std::string name;
        bool higher_is_better{false};
        BenchmarkMetricThresholdDescriptor threshold;
    };

    struct BenchmarkScenarioDescriptor
    {
        std::string identifier;
        std::string name;
        std::string dataset;
        std::string rendering_preset;
        std::string runtime_profile;
        BenchmarkCommandDescriptor engine;
        BenchmarkCommandDescriptor reference;
        std::vector<BenchmarkMetricDescriptor> metrics;
    };

    struct BenchmarkConfigurationDescriptor
    {
        int schema_version{0};
        std::vector<BenchmarkScenarioDescriptor> scenarios;
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
        std::string selected_algorithm_variant;
        std::string shading_mode;
        std::unordered_map<std::string, bool> overlays;
        int resolution_width{0};
        int resolution_height{0};
        int benchmark_frames{600};
        float benchmark_timestep{1.0F / 60.0F};
        std::string selected_case_study;
    };

    struct ExperimentConfigurationSummary
    {
        std::vector<DatasetDescriptor> datasets;
        std::optional<std::string> selected_dataset;
        std::vector<RenderingPresetDescriptor> rendering_presets;
        std::optional<std::string> selected_rendering_preset;
        std::vector<AlgorithmVariantDescriptor> algorithm_variants;
        std::optional<std::string> selected_algorithm_variant;
        RuntimeSummary runtime;
        std::optional<TelemetryConfigurationDescriptor> telemetry;
        std::optional<BenchmarkConfigurationDescriptor> benchmarks;
        std::vector<CaseStudyDescriptor> case_studies;
        std::optional<std::string> selected_case_study;
    };

    struct SandboxCallbacks
    {
        std::function<void(const std::string& dataset_id)> on_dataset_selected;
        std::function<void(const std::string& variant_id)> on_algorithm_selected;
        std::function<void(const SandboxPreferences& preferences)> on_rendering_changed;
        std::function<SandboxBenchmarkResult(const SandboxPreferences& preferences)> on_run_benchmark;
        std::function<void(const std::string& case_study_id)> on_case_study_selected;
        std::function<void(const CaseStudyDescriptor& descriptor)> on_case_study_requested;
    };

    class ENGINE_TOOLS_API ExperimentSandbox
    {
    public:
        ExperimentSandbox();

        void set_configuration(const ExperimentConfigurationSummary& summary);
        void update_telemetry(const TelemetrySnapshot& telemetry);

        void set_callbacks(SandboxCallbacks callbacks);
        void set_comparative_benchmark_runner(std::shared_ptr<class ComparativeBenchmarkRunner> runner);

        void render();

        [[nodiscard]] const SandboxPreferences& preferences() const noexcept;
        [[nodiscard]] const TelemetrySnapshot& telemetry_snapshot() const noexcept;
        void set_preferences(const SandboxPreferences& preferences);

        [[nodiscard]] std::optional<SandboxBenchmarkResult> run_active_benchmark();

        /**
         * @brief Select an algorithm variant programmatically and emit the callback.
         */
        bool select_algorithm_variant(std::string_view variant_identifier);

        /**
         * @brief Select a dataset programmatically and emit the selection callback.
         *
         * @return true when the dataset identifier exists in the current summary.
         */
        bool select_dataset(std::string_view dataset_identifier);

        /**
         * @brief Select a rendering preset programmatically and emit the rendering callback.
         *
         * Resets shading mode and overlay preferences to preset defaults when the
         * current preferences are incompatible with the new preset.
         *
         * @return true when the preset identifier exists in the current summary.
         */
        bool select_rendering_preset(std::string_view preset_identifier);

        /**
         * @brief Override the active shading mode if supported by the current preset.
         *
         * @return true when the shading mode was applied; false if the mode was invalid.
         */
        bool set_shading_mode(std::string_view shading_mode);

        /**
         * @brief Toggle an overlay for the active preset.
         *
         * @return true when the overlay exists on the active preset; false otherwise.
         */
        bool set_overlay_enabled(std::string_view overlay_key, bool enabled);

        /**
         * @brief Request loading the specified case study via the configured callback.
         */
        bool request_case_study(std::string_view case_study_identifier);

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
        bool sync_overlay_preferences();
        void notify_preference_changes(const SandboxPreferences& previous);
        [[nodiscard]] const BenchmarkScenarioDescriptor* find_active_benchmark_scenario() const;

        ExperimentConfigurationSummary summary_{};
        TelemetrySnapshot telemetry_{};
        SandboxCallbacks callbacks_{};
        SandboxPreferences preferences_{};
        std::optional<SandboxBenchmarkResult> last_benchmark_result_{};
        std::shared_ptr<ComparativeBenchmarkRunner> comparative_runner_{};

        std::unordered_map<std::string, std::size_t> dataset_lookup_{};
        std::unordered_map<std::string, std::size_t> preset_lookup_{};
        std::unordered_map<std::string, std::size_t> algorithm_lookup_{};
        std::unordered_map<std::string, std::size_t> case_study_lookup_{};

        int selected_dataset_index_{-1};
        int selected_preset_index_{-1};
        int selected_algorithm_index_{-1};
        int selected_case_study_index_{-1};

        std::array<char, 128> dataset_filter_buffer_{};
        std::string dataset_filter_{};
        std::string dataset_filter_lower_{};
    };
} // namespace engine::tools::sandbox

