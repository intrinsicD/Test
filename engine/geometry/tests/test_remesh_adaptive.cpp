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

    TEST(RemeshAdaptive, RefinesEdgesWithinSurfaceBudget)
    {
        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kAdaptive;
        request.targets.target_edge_length = 0.5F;
        request.targets.maximum_surface_deviation = 0.4F;
        request.max_iterations = 6U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        EXPECT_GE(output.mesh.positions.size(), mesh.positions.size());
        EXPECT_LE(output.statistics.max_error,
                  request.targets.maximum_surface_deviation.value() + 1e-3F);

        const MeshEdgeStatistics stats = ComputeMeshEdgeStatistics(output.mesh);
        ASSERT_GT(stats.edge_count, 0U);
        const float allowed = request.targets.target_edge_length.value() +
                              request.targets.maximum_surface_deviation.value();
        EXPECT_LE(stats.max_edge_length, allowed + 1e-3F);
    }

    TEST(RemeshAdaptive, ComputesErrorAgainstDerivedMeanWhenNoTargetProvided)
    {
        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kAdaptive;
        request.targets.maximum_surface_deviation = 0.25F;
        request.max_iterations = 4U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.0F;

        const RemeshResult<ResolvedRemeshingTargets> resolved = ResolveRemeshingTargets(request);
        ASSERT_TRUE(resolved.has_value());

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        EXPECT_LE(output.statistics.iteration_count, request.max_iterations);

        const MeshEdgeStatistics stats = ComputeMeshEdgeStatistics(output.mesh);
        if (stats.edge_count > 0U)
        {
            const ResolvedRemeshingTargets& baseline_targets = resolved.value();
            const float baseline = baseline_targets.target_edge_length.value_or(
                baseline_targets.edge_statistics.mean_edge_length());
            const float expected = std::max(std::fabs(stats.max_edge_length - baseline),
                                            std::fabs(stats.min_edge_length - baseline));
            EXPECT_NEAR(output.statistics.max_error, expected, 1e-4F);
        }
    }
} // namespace engine::geometry
