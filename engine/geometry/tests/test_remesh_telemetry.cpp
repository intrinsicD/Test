#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"
#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/remesh/telemetry.hpp"

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

    TEST(RemeshTelemetry, RecordsUniformRemeshMetrics)
    {
        auto& telemetry = RemeshTelemetry::instance();
        telemetry.reset_for_testing();

        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 0.5F;
        request.max_iterations = 6U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;
        request.job_label = std::string{"unit-square"};

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value()) << result.error().message();

        const RemeshTelemetrySnapshot snapshot = telemetry.snapshot();
        const RemeshTelemetryOperationSnapshot& metrics = snapshot.operation(RemeshingMode::kUniform);

        EXPECT_EQ(metrics.invocations, 1U);
        EXPECT_EQ(metrics.total_iterations, metrics.last_iterations);
        EXPECT_EQ(metrics.last_iterations, result.value().statistics.iteration_count);
        EXPECT_EQ(metrics.last_vertex_count, result.value().mesh.positions.size());
        EXPECT_GE(metrics.total_splits, metrics.last_splits);
        EXPECT_GT(metrics.last_splits, 0U);
        EXPECT_GE(metrics.total_collapses, metrics.last_collapses);
        EXPECT_FALSE(metrics.last_job_label.empty());
        EXPECT_EQ(metrics.last_job_label, "unit-square");
        EXPECT_GE(metrics.last_duration_ms, 0.0);
        EXPECT_GE(metrics.max_duration_ms, metrics.last_duration_ms);
        EXPECT_GE(metrics.max_vertex_count, metrics.last_vertex_count);
    }

    TEST(RemeshTelemetry, ResetClearsMetrics)
    {
        auto& telemetry = RemeshTelemetry::instance();
        telemetry.reset_for_testing();

        const RemeshTelemetrySnapshot snapshot = telemetry.snapshot();
        const RemeshTelemetryOperationSnapshot& metrics = snapshot.operation(RemeshingMode::kUniform);

        EXPECT_EQ(metrics.invocations, 0U);
        EXPECT_EQ(metrics.total_iterations, 0U);
        EXPECT_EQ(metrics.total_splits, 0U);
        EXPECT_EQ(metrics.total_collapses, 0U);
        EXPECT_EQ(metrics.last_iterations, 0U);
        EXPECT_EQ(metrics.last_splits, 0U);
        EXPECT_EQ(metrics.last_collapses, 0U);
        EXPECT_EQ(metrics.last_vertex_count, 0U);
        EXPECT_EQ(metrics.max_vertex_count, 0U);
        EXPECT_EQ(metrics.last_duration_ms, 0.0);
        EXPECT_EQ(metrics.max_duration_ms, 0.0);
        EXPECT_TRUE(metrics.last_job_label.empty());
    }
} // namespace engine::geometry

