#include "engine/geometry/remesh/remesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_set>

#include "engine/math/math.hpp"
#include "engine/math/utils/utils.hpp"

namespace engine::geometry
{
    namespace
    {
        [[nodiscard]] RemeshValidationResult make_target_error(std::string message)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_target_configuration, std::move(message))};
        }

        [[nodiscard]] RemeshValidationResult make_attribute_error(std::string message)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_attribute_policy, std::move(message))};
        }

        [[nodiscard]] RemeshValidationResult make_parameterization_error(std::string message)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_parameterization, std::move(message))};
        }

        [[nodiscard]] bool is_positive_finite(float value) noexcept
        {
            return std::isfinite(value) && value > 0.0f;
        }

        [[nodiscard]] bool is_non_negative_finite(float value) noexcept
        {
            return std::isfinite(value) && value >= 0.0f;
        }
    } // namespace

    RemeshValidationResult ValidateRemeshRequest(const RemeshRequest& request) noexcept
    {
        if (request.input_mesh == nullptr)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                            "remesh request missing input mesh")};
        }

        if (request.input_mesh->positions.empty())
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                            "input mesh must contain vertices")};
        }

        if (!request.input_mesh->indices.empty() && request.input_mesh->indices.size() % 3 != 0)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                            "triangle index buffer must be a multiple of three")};
        }

        for (const std::uint32_t index : request.input_mesh->indices)
        {
            if (index >= request.input_mesh->positions.size())
            {
                return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                                "triangle index out of range")};
            }
        }

        if (request.max_iterations == 0U)
        {
            return make_target_error("max_iterations must be greater than zero");
        }

        if (!is_positive_finite(request.relaxation_factor) || request.relaxation_factor > 1.0f)
        {
            return make_target_error("relaxation_factor must be within (0, 1]");
        }

        if (!is_non_negative_finite(request.tangential_smoothing_weight) ||
            request.tangential_smoothing_weight > 1.0f)
        {
            return make_target_error("tangential_smoothing_weight must be within [0, 1]");
        }

        bool has_primary_target = false;
        if (request.targets.target_edge_length.has_value())
        {
            const float value = request.targets.target_edge_length.value();
            if (!is_positive_finite(value))
            {
                return make_target_error("target_edge_length must be positive and finite");
            }
            has_primary_target = true;
        }

        if (request.targets.relative_edge_scale.has_value())
        {
            const float value = request.targets.relative_edge_scale.value();
            if (!is_positive_finite(value))
            {
                return make_target_error("relative_edge_scale must be positive and finite");
            }
            has_primary_target = true;
        }

        bool has_adaptive_budget = false;
        if (request.targets.maximum_normal_deviation_degrees.has_value())
        {
            const float value = request.targets.maximum_normal_deviation_degrees.value();
            if (!std::isfinite(value) || value <= 0.0f || value > 180.0f)
            {
                return make_target_error("maximum_normal_deviation_degrees must lie within (0, 180]");
            }
            has_adaptive_budget = true;
        }

        if (request.targets.maximum_surface_deviation.has_value())
        {
            const float value = request.targets.maximum_surface_deviation.value();
            if (!is_non_negative_finite(value))
            {
                return make_target_error("maximum_surface_deviation must be non-negative and finite");
            }
            has_adaptive_budget = true;
        }

        switch (request.mode)
        {
        case RemeshingMode::kUniform:
            if (!has_primary_target)
            {
                return make_target_error("uniform remeshing requires an edge length target");
            }
            break;
        case RemeshingMode::kFeaturePreserving:
            if (!has_primary_target)
            {
                return make_target_error("feature-preserving remeshing requires an edge length target");
            }
            if (!std::isfinite(request.feature_preservation.minimum_feature_angle_degrees) ||
                request.feature_preservation.minimum_feature_angle_degrees <= 0.0f ||
                request.feature_preservation.minimum_feature_angle_degrees >= 180.0f)
            {
                return make_target_error("minimum_feature_angle_degrees must lie within (0, 180)");
            }
            break;
        case RemeshingMode::kAdaptive:
            if (!has_primary_target && !has_adaptive_budget)
            {
                return make_target_error("adaptive remeshing requires either edge targets or error budgets");
            }
            if (!has_adaptive_budget)
            {
                return make_target_error("adaptive remeshing requires an error budget");
            }
            break;
        }

        if (request.attribute_policy.positions != AttributeTransferMode::kPreserve)
        {
            return make_attribute_error("position attributes must be preserved during remeshing");
        }

        if (request.attribute_policy.skinning_weights == AttributeTransferMode::kDrop)
        {
            return make_attribute_error("skinning_weights cannot be dropped during remeshing");
        }

        if (request.parameterization.mode == ParameterizationMode::kReuseExisting &&
            request.attribute_policy.texture_coordinates == AttributeTransferMode::kDrop)
        {
            return make_attribute_error(
                "texture coordinates cannot be dropped when parameterisation reuses existing data");
        }

        if (!is_non_negative_finite(request.parameterization.gutter_width))
        {
            return make_parameterization_error("gutter_width must be non-negative and finite");
        }

        switch (request.parameterization.mode)
        {
        case ParameterizationMode::kNone:
            break;
        case ParameterizationMode::kReuseExisting:
            if (!is_non_negative_finite(request.parameterization.target_texel_density))
            {
                return make_parameterization_error(
                    "target_texel_density must be non-negative when reusing parameterisation");
            }
            break;
        case ParameterizationMode::kGenerateLscm:
        case ParameterizationMode::kGenerateAbfpp:
            if (!is_positive_finite(request.parameterization.target_texel_density))
            {
                return make_parameterization_error(
                    "target_texel_density must be positive when generating parameterisation");
            }
            break;
        }

        return RemeshValidationResult{};
    }

    namespace
    {
        [[nodiscard]] std::uint64_t make_edge_key(std::uint32_t a, std::uint32_t b) noexcept
        {
            if (a > b)
            {
                std::swap(a, b);
            }
            return (static_cast<std::uint64_t>(a) << 32U) | static_cast<std::uint64_t>(b);
        }
    } // namespace

    MeshEdgeStatistics ComputeMeshEdgeStatistics(const SurfaceMesh& mesh) noexcept
    {
        MeshEdgeStatistics statistics{};

        if (mesh.positions.size() < 2U || mesh.indices.empty())
        {
            statistics.min_edge_length = 0.0f;
            return statistics;
        }

        std::unordered_set<std::uint64_t> unique_edges{};
        unique_edges.reserve(mesh.indices.size());

        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            const std::uint32_t a = mesh.indices[i];
            const std::uint32_t b = mesh.indices[i + 1];
            const std::uint32_t c = mesh.indices[i + 2];

            const std::array<std::pair<std::uint32_t, std::uint32_t>, 3> edges{{
                {a, b},
                {b, c},
                {c, a},
            }};

            for (const auto& [u, v] : edges)
            {
                if (u == v)
                {
                    continue;
                }

                const std::uint64_t key = make_edge_key(u, v);
                if (!unique_edges.insert(key).second)
                {
                    continue;
                }

                const math::vec3& p0 = mesh.positions[u];
                const math::vec3& p1 = mesh.positions[v];
                const float length = math::length(p1 - p0);

                statistics.total_edge_length += length;
                statistics.max_edge_length = std::max(statistics.max_edge_length, length);
                statistics.min_edge_length = std::min(statistics.min_edge_length, length);
                ++statistics.edge_count;
            }
        }

        if (statistics.edge_count == 0U)
        {
            statistics.min_edge_length = 0.0f;
            statistics.max_edge_length = 0.0f;
            statistics.total_edge_length = 0.0f;
        }

        return statistics;
    }

    RemeshResult<ResolvedRemeshingTargets> ResolveRemeshingTargets(const RemeshRequest& request) noexcept
    {
        if (const RemeshValidationResult validation = ValidateRemeshRequest(request); !validation.has_value())
        {
            return RemeshResult<ResolvedRemeshingTargets>{validation.error()};
        }

        ResolvedRemeshingTargets resolved{};
        resolved.edge_statistics = ComputeMeshEdgeStatistics(*request.input_mesh);

        if (resolved.edge_statistics.edge_count == 0U)
        {
            resolved.edge_statistics.min_edge_length = 0.0f;
        }

        if (request.targets.maximum_normal_deviation_degrees.has_value())
        {
            resolved.maximum_normal_deviation_degrees = request.targets.maximum_normal_deviation_degrees;
        }

        if (request.targets.maximum_surface_deviation.has_value())
        {
            resolved.maximum_surface_deviation = request.targets.maximum_surface_deviation;
        }

        if (request.targets.target_edge_length.has_value())
        {
            resolved.target_edge_length = request.targets.target_edge_length;
        }

        if (request.targets.relative_edge_scale.has_value())
        {
            const float mean_length = resolved.edge_statistics.mean_edge_length();
            if (mean_length <= std::numeric_limits<float>::min())
            {
                return RemeshResult<ResolvedRemeshingTargets>{make_remesh_error(
                    RemeshError::invalid_target_configuration,
                    "relative_edge_scale requires a mesh with non-degenerate edges")};
            }

            const float derived_edge_length =
                mean_length * request.targets.relative_edge_scale.value();

            if (resolved.target_edge_length.has_value())
            {
                if (!math::utils::nearly_equal(resolved.target_edge_length.value(), derived_edge_length, 1e-4f))
                {
                    return RemeshResult<ResolvedRemeshingTargets>{make_remesh_error(
                        RemeshError::invalid_target_configuration,
                        "absolute and relative edge targets disagree")};
                }
            }
            else
            {
                resolved.target_edge_length = derived_edge_length;
            }
        }

        if (!resolved.target_edge_length.has_value() && request.mode != RemeshingMode::kAdaptive)
        {
            // Validation should have caught this already, but provide a defensive guard for callers
            // that bypass ValidateRemeshRequest.
            return RemeshResult<ResolvedRemeshingTargets>{make_remesh_error(
                RemeshError::invalid_target_configuration,
                "remeshing mode requires an absolute edge length target")};
        }

        return RemeshResult<ResolvedRemeshingTargets>{resolved};
    }
} // namespace engine::geometry

