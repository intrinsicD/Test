#pragma once

#include "engine/core/diagnostics/result.hpp"
#include "engine/geometry/remesh/remesh.hpp"

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>

namespace engine::geometry::tools
{
    struct DatasetFileDigest
    {
        std::filesystem::path path{};
        std::uintmax_t size_bytes{0U};
        std::string sha256{};
    };

    struct RemeshCliOptions
    {
        std::filesystem::path input_path{};
        std::filesystem::path output_path{};
        RemeshingMode mode{RemeshingMode::kUniform};
        RemeshingTargets targets{};
        FeaturePreservationOptions feature_preservation{};
        ParameterizationPolicy parameterization{};
        std::uint32_t max_iterations{64U};
        float relaxation_factor{0.6F};
        float tangential_smoothing_weight{0.5F};
        bool record_diagnostics{true};
        bool verbose{false};
        std::optional<std::string> job_label{};
        std::optional<std::filesystem::path> manifest_output_path{};
        bool show_help{false};
    };

    struct RemeshCliExecutionResult
    {
        RemeshOutput output{};
        std::size_t input_vertex_count{0U};
        std::size_t input_face_count{0U};
        MeshEdgeStatistics input_edge_statistics{};
        float input_surface_area{0.0F};
        float output_surface_area{0.0F};
        DatasetFileDigest input_file{};
        DatasetFileDigest output_file{};
    };

    using RemeshCliOptionsResult = engine::Result<RemeshCliOptions, std::string>;
    using RemeshCliExecution = engine::Result<RemeshCliExecutionResult, std::string>;

    RemeshCliOptionsResult ParseArguments(std::span<const char* const> arguments) noexcept;

    RemeshCliExecution ExecuteRemesh(const RemeshCliOptions& options) noexcept;

    void PrintSummary(const RemeshCliOptions& options,
                      const RemeshCliExecutionResult& result,
                      std::ostream& stream) noexcept;

    void PrintHelp(std::ostream& stream) noexcept;

    [[nodiscard]] std::string BuildDatasetManifestEntry(const RemeshCliOptions& options,
                                                        const RemeshCliExecutionResult& result) noexcept;
}