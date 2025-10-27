#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/api.hpp"
#include "engine/math/utils/utils.hpp"

namespace engine::geometry
{
    namespace
    {
        SurfaceMesh make_unit_square_mesh()
        {
            SurfaceMesh mesh{};
            mesh.positions = {
                {0.0F, 0.0F, 0.0F},
                {1.0F, 0.0F, 0.0F},
                {0.0F, 1.0F, 0.0F},
                {1.0F, 1.0F, 0.0F},
            };
            mesh.rest_positions = mesh.positions;
            mesh.indices = {
                0U, 1U, 2U,
                2U, 1U, 3U,
            };
            return mesh;
        }
    } // namespace

    TEST(RemeshUniform, SplitsLongEdges)
    {
        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 0.25F;
        request.max_iterations = 6U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        EXPECT_GT(output.mesh.positions.size(), mesh.positions.size());

        const MeshEdgeStatistics stats = ComputeMeshEdgeStatistics(output.mesh);
        EXPECT_LT(stats.max_edge_length, request.targets.target_edge_length.value() * 1.75F);
        EXPECT_GT(output.statistics.iteration_count, 0U);
        EXPECT_GT(output.statistics.split_count, 0U);
        EXPECT_GE(output.statistics.duration_ms, 0.0);
    }

    TEST(RemeshUniform, ReportsMaxErrorAgainstTarget)
    {
        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 0.4F;
        request.max_iterations = 5U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        const MeshEdgeStatistics stats = ComputeMeshEdgeStatistics(output.mesh);
        const float target = request.targets.target_edge_length.value();
        const float expected = std::max(std::fabs(stats.max_edge_length - target),
                                        std::fabs(stats.min_edge_length - target));

        EXPECT_GE(output.statistics.max_error, 0.0F);
        EXPECT_NEAR(output.statistics.max_error, expected, 1e-4F);
        EXPECT_GE(output.statistics.max_surface_deviation, 0.0F);
        EXPECT_GE(output.statistics.mean_surface_deviation, 0.0F);
        EXPECT_GE(output.statistics.rms_surface_deviation, 0.0F);
        EXPECT_GE(output.statistics.duration_ms, 0.0);
    }


    TEST(RemeshUniform, RestPositionsMatchPositionsAfterRemesh)
    {
        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 0.25F;
        request.max_iterations = 6U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        ASSERT_EQ(output.mesh.positions.size(), output.mesh.rest_positions.size());
        ASSERT_GT(output.mesh.positions.size(), mesh.positions.size());
        for (std::size_t index = 0; index < output.mesh.positions.size(); ++index)
        {
            EXPECT_NEAR(output.mesh.rest_positions[index][0], output.mesh.positions[index][0], 1e-5F);
            EXPECT_NEAR(output.mesh.rest_positions[index][1], output.mesh.positions[index][1], 1e-5F);
            EXPECT_NEAR(output.mesh.rest_positions[index][2], output.mesh.positions[index][2], 1e-5F);
        }
    }

    TEST(RemeshUniform, RestPositionsMatchPositionsAfterCollapses)
    {
        SurfaceMesh mesh{};
        mesh.positions = {
            {0.0F, 0.0F, 0.0F},
            {1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
            {1.0F, 1.0F, 0.0F},
            {0.5F, 0.5F, 0.0F},
        };
        mesh.rest_positions = mesh.positions;
        mesh.indices = {
            0U, 1U, 4U,
            1U, 3U, 4U,
            3U, 2U, 4U,
            2U, 0U, 4U,
        };

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 2.0F;
        request.max_iterations = 6U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        ASSERT_EQ(output.mesh.positions.size(), output.mesh.rest_positions.size());
        EXPECT_LE(output.mesh.positions.size(), mesh.positions.size());
        for (std::size_t index = 0; index < output.mesh.positions.size(); ++index)
        {
            EXPECT_NEAR(output.mesh.rest_positions[index][0], output.mesh.positions[index][0], 1e-5F);
            EXPECT_NEAR(output.mesh.rest_positions[index][1], output.mesh.positions[index][1], 1e-5F);
            EXPECT_NEAR(output.mesh.rest_positions[index][2], output.mesh.positions[index][2], 1e-5F);
        }
    }

    TEST(RemeshUniform, ReuseParameterizationScalesTexelDensity)
    {
        SurfaceMesh mesh = make_unit_square_mesh();
        mesh.texture_coordinates = {
            {0.0F, 0.0F},
            {1.0F, 0.0F},
            {0.0F, 1.0F},
            {1.0F, 1.0F},
        };

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 1.2F;
        request.parameterization.mode = ParameterizationMode::kReuseExisting;
        request.parameterization.target_texel_density = 2.0F;
        request.parameterization.gutter_width = 0.0F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        ASSERT_EQ(output.mesh.texture_coordinates.size(), output.mesh.positions.size());
        EXPECT_EQ(output.parameterization.chart_count, 1U);
        EXPECT_NEAR(output.parameterization.texel_density, 2.0F, 1e-3F);
        ASSERT_GT(output.mesh.texture_coordinates.size(), 1U);
        EXPECT_NEAR(output.mesh.texture_coordinates[1][0], 2.0F, 1e-3F);
        EXPECT_NEAR(output.mesh.texture_coordinates[1][1], 0.0F, 1e-3F);
    }
} // namespace engine::geometry