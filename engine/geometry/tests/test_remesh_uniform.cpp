#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/api.hpp"

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
    }

    TEST(RemeshUniform, UnsupportedModeReportsError)
    {
        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kAdaptive;
        request.targets.maximum_surface_deviation = 0.1F;
        request.targets.relative_edge_scale = 1.0F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(static_cast<int>(result.error().value()), static_cast<int>(RemeshError::unsupported_mode));
    }
} // namespace engine::geometry

