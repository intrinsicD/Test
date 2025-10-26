#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/export.hpp"
#include "engine/geometry/remesh/errors.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <limits>
#include <vector>

namespace engine::geometry
{
    enum class RemeshingMode : std::uint8_t
    {
        kUniform = 0U,
        kFeaturePreserving,
        kAdaptive
    };

    enum class AttributeTransferMode : std::uint8_t
    {
        kPreserve = 0U,
        kResample,
        kDrop
    };

    struct ENGINE_GEOMETRY_API AttributeTransferPolicy
    {
        AttributeTransferMode positions{AttributeTransferMode::kPreserve};
        AttributeTransferMode normals{AttributeTransferMode::kResample};
        AttributeTransferMode tangents{AttributeTransferMode::kResample};
        AttributeTransferMode bitangents{AttributeTransferMode::kResample};
        AttributeTransferMode texture_coordinates{AttributeTransferMode::kResample};
        AttributeTransferMode colors{AttributeTransferMode::kPreserve};
        AttributeTransferMode skinning_weights{AttributeTransferMode::kPreserve};
    };

    struct ENGINE_GEOMETRY_API RemeshingTargets
    {
        std::optional<float> target_edge_length{};            ///< Absolute edge length in world units.
        std::optional<float> relative_edge_scale{};           ///< Edge length multiplier relative to input mean.
        std::optional<float> maximum_normal_deviation_degrees{}; ///< Maximum allowed deviation in degrees.
        std::optional<float> maximum_surface_deviation{};     ///< Allowed Hausdorff-style deviation in world units.
    };

    struct ENGINE_GEOMETRY_API FeaturePreservationOptions
    {
        bool lock_boundary_edges{true};
        bool lock_feature_edges{true};
        float minimum_feature_angle_degrees{45.0f};
    };

    enum class ParameterizationMode : std::uint8_t
    {
        kNone = 0U,
        kReuseExisting,
        kGenerateLscm,
        kGenerateAbfpp
    };

    struct ENGINE_GEOMETRY_API ParameterizationPolicy
    {
        ParameterizationMode mode{ParameterizationMode::kNone};
        float target_texel_density{0.0f};
        float gutter_width{2.0f};
        bool repack_islands{true};
        bool allow_chart_reuse{true};
    };

    struct ENGINE_GEOMETRY_API ParameterizationChart
    {
        math::vec2 min_uv{0.0F, 0.0F};
        math::vec2 max_uv{0.0F, 0.0F};
        math::vec2 translation{0.0F, 0.0F};
        float scale{1.0F};
        float area{0.0F};
    };

    struct ENGINE_GEOMETRY_API RemeshRequest
    {
        const SurfaceMesh* input_mesh{nullptr};
        RemeshingMode mode{RemeshingMode::kUniform};
        RemeshingTargets targets{};
        FeaturePreservationOptions feature_preservation{};
        AttributeTransferPolicy attribute_policy{};
        ParameterizationPolicy parameterization{};
        std::uint32_t max_iterations{64};
        float relaxation_factor{0.6f};
        float tangential_smoothing_weight{0.5f};
        bool record_diagnostics{true};
        std::optional<std::string> job_label{};
    };

    struct ENGINE_GEOMETRY_API RemeshStatistics
    {
        std::uint32_t iteration_count{0};
        float max_edge_length{0.0f};
        float min_edge_length{0.0f};
        float max_error{0.0f};
    };

    struct ENGINE_GEOMETRY_API ParameterizationSummary
    {
        std::uint32_t chart_count{0};
        float average_stretch{0.0f};
        float max_stretch{0.0f};
        float texel_density{0.0f};
        std::vector<ParameterizationChart> charts{};
    };

    struct ENGINE_GEOMETRY_API RemeshOutput
    {
        SurfaceMesh mesh{};
        ParameterizationSummary parameterization{};
        RemeshStatistics statistics{};
    };

    struct ENGINE_GEOMETRY_API MeshEdgeStatistics
    {
        std::uint32_t edge_count{0U};
        float min_edge_length{std::numeric_limits<float>::infinity()};
        float max_edge_length{0.0f};
        float total_edge_length{0.0f};

        [[nodiscard]] float mean_edge_length() const noexcept
        {
            return edge_count > 0U ? total_edge_length / static_cast<float>(edge_count) : 0.0f;
        }
    };

    struct ENGINE_GEOMETRY_API ResolvedRemeshingTargets
    {
        MeshEdgeStatistics edge_statistics{};
        std::optional<float> target_edge_length{};
        std::optional<float> maximum_normal_deviation_degrees{};
        std::optional<float> maximum_surface_deviation{};
    };

    [[nodiscard]] ENGINE_GEOMETRY_API RemeshValidationResult ValidateRemeshRequest(const RemeshRequest& request) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API MeshEdgeStatistics ComputeMeshEdgeStatistics(const SurfaceMesh& mesh) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API RemeshResult<ResolvedRemeshingTargets> ResolveRemeshingTargets(
        const RemeshRequest& request) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API RemeshResult<RemeshOutput> Remesh(const RemeshRequest& request) noexcept;
}

