#include "engine/geometry/remesh/remesh.hpp"

#include <cmath>
#include <string>

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
} // namespace engine::geometry

