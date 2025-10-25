#include <gtest/gtest.h>

#include "engine/geometry/remesh/remesh.hpp"

namespace engine::geometry
{
    namespace
    {
        SurfaceMesh MakeSimpleMesh()
        {
            SurfaceMesh mesh{};
            mesh.positions = {
                {0.0f, 0.0f, 0.0f},
                {1.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f},
            };
            mesh.indices = {0U, 1U, 2U};
            return mesh;
        }
    } // namespace

    TEST(RemeshRequestValidation, AcceptsUniformEdgeLength)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 0.1f;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        EXPECT_TRUE(result.has_value());
    }

    TEST(RemeshRequestValidation, RejectsMissingInputMesh)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.targets.target_edge_length = 0.1f;
        request.input_mesh = nullptr;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_input_mesh);
    }

    TEST(RemeshRequestValidation, RejectsUniformWithoutTarget)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_target_configuration);
    }

    TEST(RemeshRequestValidation, RejectsNegativeEdgeLength)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = -0.5f;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_target_configuration);
    }

    TEST(RemeshRequestValidation, AdaptiveRequiresBudget)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kAdaptive;
        request.targets.target_edge_length = 0.2f;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_target_configuration);
    }

    TEST(RemeshRequestValidation, FeaturePreservingRequiresValidAngle)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kFeaturePreserving;
        request.targets.target_edge_length = 0.2f;
        request.feature_preservation.minimum_feature_angle_degrees = 0.0f;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_target_configuration);
    }

    TEST(RemeshRequestValidation, RejectsDroppingPositions)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.2f;
        request.attribute_policy.positions = AttributeTransferMode::kDrop;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_attribute_policy);
    }

    TEST(RemeshRequestValidation, RejectsDroppingSkinningWeights)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.2f;
        request.attribute_policy.skinning_weights = AttributeTransferMode::kDrop;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_attribute_policy);
    }

    TEST(RemeshRequestValidation, ParameterisationGenerationRequiresDensity)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.2f;
        request.parameterization.mode = ParameterizationMode::kGenerateLscm;
        request.parameterization.target_texel_density = 0.0f;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_parameterization);
    }

    TEST(RemeshRequestValidation, ReuseParameterisationCannotDropUvs)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.2f;
        request.parameterization.mode = ParameterizationMode::kReuseExisting;
        request.attribute_policy.texture_coordinates = AttributeTransferMode::kDrop;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_attribute_policy);
    }
}

