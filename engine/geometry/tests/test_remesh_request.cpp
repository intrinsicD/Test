#include <cmath>

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

    TEST(RemeshRequestValidation, RejectsOutOfRangeIndices)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        mesh.indices = {0U, 1U, 42U};

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.2f;

        const RemeshValidationResult result = ValidateRemeshRequest(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RemeshError::invalid_input_mesh);
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

    TEST(RemeshTargets, ComputesEdgeStatisticsForSingleTriangle)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        const MeshEdgeStatistics statistics = ComputeMeshEdgeStatistics(mesh);

        EXPECT_EQ(statistics.edge_count, 3U);
        EXPECT_FLOAT_EQ(statistics.max_edge_length, std::sqrt(2.0f));
        EXPECT_FLOAT_EQ(statistics.min_edge_length, 1.0f);
        EXPECT_NEAR(statistics.mean_edge_length(), (2.0f + std::sqrt(2.0f)) / 3.0f, 1e-6f);
    }

    TEST(RemeshTargets, ComputesEdgeStatisticsWithoutDuplicates)
    {
        SurfaceMesh mesh{};
        mesh.positions = {
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f},
        };
        mesh.indices = {
            0U, 1U, 2U,
            2U, 1U, 3U,
        };

        const MeshEdgeStatistics statistics = ComputeMeshEdgeStatistics(mesh);
        EXPECT_EQ(statistics.edge_count, 5U);
        EXPECT_FLOAT_EQ(statistics.min_edge_length, 1.0f);
        EXPECT_FLOAT_EQ(statistics.max_edge_length, std::sqrt(2.0f));
    }

    TEST(RemeshTargets, ResolvesAbsoluteTargetWithoutRelativeScale)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.2f;

        const RemeshResult<ResolvedRemeshingTargets> resolved = ResolveRemeshingTargets(request);
        ASSERT_TRUE(resolved.has_value());
        const ResolvedRemeshingTargets& targets = resolved.value();
        ASSERT_TRUE(targets.target_edge_length.has_value());
        EXPECT_FLOAT_EQ(targets.target_edge_length.value(), 0.2f);
    }

    TEST(RemeshTargets, ResolvesRelativeScaleIntoAbsoluteEdgeLength)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.relative_edge_scale = 0.5f;
        request.targets.target_edge_length.reset();

        const RemeshResult<ResolvedRemeshingTargets> resolved = ResolveRemeshingTargets(request);
        ASSERT_TRUE(resolved.has_value());
        const ResolvedRemeshingTargets& targets = resolved.value();
        ASSERT_TRUE(targets.target_edge_length.has_value());
        const float expected = ComputeMeshEdgeStatistics(mesh).mean_edge_length() * 0.5f;
        EXPECT_NEAR(targets.target_edge_length.value(), expected, 1e-6f);
    }

    TEST(RemeshTargets, RejectsConflictingTargets)
    {
        SurfaceMesh mesh = MakeSimpleMesh();
        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.target_edge_length = 0.1f;
        request.targets.relative_edge_scale = 10.0f;

        const RemeshResult<ResolvedRemeshingTargets> resolved = ResolveRemeshingTargets(request);
        ASSERT_FALSE(resolved.has_value());
        EXPECT_EQ(resolved.error().code(), RemeshError::invalid_target_configuration);
    }

    TEST(RemeshTargets, RejectsRelativeScaleWhenMeshHasNoTriangles)
    {
        SurfaceMesh mesh{};
        mesh.positions = {{0.0f, 0.0f, 0.0f}};

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.targets.relative_edge_scale = 1.5f;

        const RemeshResult<ResolvedRemeshingTargets> resolved = ResolveRemeshingTargets(request);
        ASSERT_FALSE(resolved.has_value());
        EXPECT_EQ(resolved.error().code(), RemeshError::invalid_target_configuration);
    }
}

