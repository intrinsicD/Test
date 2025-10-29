#pragma once

#include "engine/runtime/errors.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::runtime::config
{
    struct RemeshingTargets
    {
        std::optional<double> target_edge_length;
        std::optional<double> relative_edge_scale;
        std::optional<double> max_normal_deviation_degrees;
        std::optional<double> max_surface_deviation;
    };

    struct FeaturePreservation
    {
        bool lock_boundary_edges = false;
        bool lock_feature_edges = false;
        double minimum_feature_angle_degrees = 0.0;
    };

    struct EdgeLengthMetrics
    {
        double minimum = 0.0;
        double maximum = 0.0;
        double mean = 0.0;
    };

    struct MeshMetrics
    {
        int vertices = 0;
        int faces = 0;
        EdgeLengthMetrics edge_length;
    };

    struct ParameterizationChart
    {
        int index = 0;
        std::array<double, 2> min_uv{};
        std::array<double, 2> max_uv{};
        std::array<double, 2> translation{};
        double scale = 1.0;
        double area = 0.0;
        double boundary_length = 0.0;
    };

    struct ParameterizationSummary
    {
        std::string mode;
        std::optional<double> target_texel_density;
        double texel_density = 0.0;
        int chart_count = 0;
        double average_stretch = 0.0;
        double max_stretch = 0.0;
        double fill_ratio = 0.0;
        double total_seam_length = 0.0;
        std::optional<double> atlas_area;
        std::optional<double> total_chart_area;
        std::vector<ParameterizationChart> charts;
    };

    struct DatasetStatistics
    {
        int iteration_count = 0;
        double max_error = 0.0;
        double min_edge_length = 0.0;
        double max_edge_length = 0.0;
        double max_surface_deviation = 0.0;
        double mean_surface_deviation = 0.0;
        double rms_surface_deviation = 0.0;
    };

    struct DatasetEntry
    {
        std::string identifier;
        std::string schema_id;
        int schema_version = 1;
        std::string kind;
        std::vector<std::string> tags;
        std::optional<std::string> job_label;

        std::string source_generator;
        std::string source_mesh;
        std::optional<std::string> source_mesh_sha256;
        std::optional<std::uint64_t> source_mesh_size_bytes;

        std::string output_mesh;
        std::optional<std::string> output_mesh_sha256;
        std::optional<std::uint64_t> output_mesh_size_bytes;

        std::string remeshing_mode;
        std::optional<RemeshingTargets> remeshing_targets;
        FeaturePreservation feature_preservation;
        MeshMetrics input_metrics;
        MeshMetrics output_metrics;
        std::optional<ParameterizationSummary> parameterization;
        DatasetStatistics statistics;
    };

    struct DatasetManifest
    {
        std::vector<DatasetEntry> datasets;
    };

    struct RenderingConfig
    {
        int schema_version = 1;
        std::string preset;
        std::string shading_mode;
        int width = 0;
        int height = 0;
        bool overlay_normals = false;
        bool overlay_uv = false;
        bool overlay_material = false;
        bool overlay_light_volume = false;
    };

    struct RuntimeCameraConfig
    {
        std::string mode;
        std::optional<std::array<double, 3>> position;
        std::optional<std::array<double, 3>> target;
    };

    struct RuntimeSimulationConfig
    {
        double timestep_seconds = 0.0;
        int max_substeps = 0;
    };

    struct RuntimeHotReloadConfig
    {
        bool enabled = false;
        std::optional<double> watch_interval_seconds;
    };

    struct RuntimeConfig
    {
        int schema_version = 1;
        std::optional<std::string> dataset;
        std::optional<std::string> scene_manifest;
        std::optional<std::string> scene_entry_point;
        std::optional<RuntimeCameraConfig> camera;
        std::optional<RuntimeSimulationConfig> simulation;
        RuntimeHotReloadConfig hot_reload;
    };

    struct BenchmarkThreshold
    {
        std::string mode;
        double limit = 0.0;
    };

    struct BenchmarkMetricConfig
    {
        std::string name;
        bool higher_is_better = false;
        BenchmarkThreshold threshold;
    };

    struct BenchmarkCommandConfig
    {
        std::optional<std::vector<std::string>> command;
        std::string output;
    };

    struct BenchmarkScenarioConfig
    {
        std::string identifier;
        std::string name;
        std::optional<std::string> dataset;
        std::optional<std::string> rendering_preset;
        std::optional<std::string> runtime_profile;
        BenchmarkCommandConfig engine;
        BenchmarkCommandConfig reference;
        std::vector<BenchmarkMetricConfig> metrics;
    };

    struct BenchmarkConfig
    {
        int schema_version = 1;
        std::vector<BenchmarkScenarioConfig> scenarios;
    };

    struct TelemetryOutputConfig
    {
        std::string kind;
        std::optional<std::string> path;
    };

    struct TelemetryMetricConfig
    {
        std::string name;
        std::string statistic;
    };

    struct TelemetrySamplingConfig
    {
        int frame_interval = 1;
        bool include_debug_overlays = false;
    };

    struct TelemetryConfig
    {
        int schema_version = 1;
        std::vector<TelemetryOutputConfig> outputs;
        std::vector<TelemetryMetricConfig> metrics;
        std::optional<TelemetrySamplingConfig> sampling;
    };

    struct Ai004Configuration
    {
        DatasetManifest datasets;
        std::optional<RenderingConfig> rendering;
        std::optional<RuntimeConfig> runtime;
        std::optional<BenchmarkConfig> benchmarks;
        std::optional<TelemetryConfig> telemetry;
    };

    [[nodiscard]] RuntimeResult<DatasetManifest> load_dataset_manifest(
        const std::filesystem::path& path,
        std::optional<bool> require_schema = std::nullopt) noexcept;

    [[nodiscard]] RuntimeResult<Ai004Configuration> load_configuration(
        const std::filesystem::path& path,
        std::optional<bool> require_schema = std::nullopt) noexcept;
}

