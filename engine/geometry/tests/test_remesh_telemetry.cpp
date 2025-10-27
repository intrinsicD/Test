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
        const RemeshOutput& output = result.value();

        const RemeshTelemetrySnapshot snapshot = telemetry.snapshot();
        const RemeshTelemetryOperationSnapshot& metrics = snapshot.operation(RemeshingMode::kUniform);

        EXPECT_EQ(metrics.invocations, 1U);
        EXPECT_EQ(metrics.total_iterations, metrics.last_iterations);
        EXPECT_EQ(metrics.last_iterations, output.statistics.iteration_count);
        EXPECT_EQ(metrics.last_vertex_count, output.mesh.positions.size());
        EXPECT_GE(metrics.total_splits, metrics.last_splits);
        EXPECT_GT(metrics.last_splits, 0U);
        EXPECT_EQ(metrics.last_splits, output.statistics.split_count);
        EXPECT_GE(metrics.total_collapses, metrics.last_collapses);
        EXPECT_EQ(metrics.last_collapses, output.statistics.collapse_count);
        EXPECT_FALSE(metrics.last_job_label.empty());
        EXPECT_EQ(metrics.last_job_label, "unit-square");
        EXPECT_GE(metrics.last_duration_ms, 0.0);
        EXPECT_NEAR(metrics.last_duration_ms, output.statistics.duration_ms, 1e-6);
        EXPECT_GE(metrics.max_duration_ms, metrics.last_duration_ms);
        EXPECT_GE(metrics.max_vertex_count, metrics.last_vertex_count);
        EXPECT_EQ(metrics.surface_deviation_invocations, 1U);
        EXPECT_GT(metrics.last_surface_deviation_sample_count, 0U);
        EXPECT_EQ(metrics.total_surface_deviation_sample_count,
                  metrics.last_surface_deviation_sample_count);
        EXPECT_NEAR(metrics.last_max_surface_deviation,
                    static_cast<double>(output.statistics.max_surface_deviation),
                    1e-6);
        EXPECT_NEAR(metrics.max_surface_deviation,
                    static_cast<double>(output.statistics.max_surface_deviation),
                    1e-6);
        EXPECT_NEAR(metrics.average_max_surface_deviation,
                    static_cast<double>(output.statistics.max_surface_deviation),
                    1e-6);
        EXPECT_NEAR(metrics.last_mean_surface_deviation,
                    static_cast<double>(output.statistics.mean_surface_deviation),
                    1e-6);
        EXPECT_NEAR(metrics.average_mean_surface_deviation,
                    static_cast<double>(output.statistics.mean_surface_deviation),
                    1e-6);
        EXPECT_NEAR(metrics.last_rms_surface_deviation,
                    static_cast<double>(output.statistics.rms_surface_deviation),
                    1e-6);
        EXPECT_NEAR(metrics.average_rms_surface_deviation,
                    static_cast<double>(output.statistics.rms_surface_deviation),
                    1e-6);
    }

    TEST(RemeshTelemetry, SkipsRecordingWhenDiagnosticsDisabled)
    {
        auto& telemetry = RemeshTelemetry::instance();
        telemetry.reset_for_testing();

        SurfaceMesh mesh = make_unit_square_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kUniform;
        request.targets.target_edge_length = 0.5F;
        request.max_iterations = 4U;
        request.record_diagnostics = false;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value()) << result.error().message();

        const RemeshTelemetrySnapshot snapshot = telemetry.snapshot();
        const RemeshTelemetryOperationSnapshot& metrics = snapshot.operation(RemeshingMode::kUniform);

        EXPECT_EQ(metrics.invocations, 0U);
        EXPECT_EQ(metrics.total_iterations, 0U);
        EXPECT_EQ(metrics.total_splits, 0U);
        EXPECT_EQ(metrics.total_collapses, 0U);
        EXPECT_EQ(metrics.surface_deviation_invocations, 0U);
        EXPECT_EQ(metrics.last_surface_deviation_sample_count, 0U);
        EXPECT_TRUE(metrics.last_job_label.empty());
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
        EXPECT_EQ(metrics.surface_deviation_invocations, 0U);
        EXPECT_EQ(metrics.last_max_surface_deviation, 0.0);
        EXPECT_EQ(metrics.max_surface_deviation, 0.0);
        EXPECT_EQ(metrics.average_max_surface_deviation, 0.0);
        EXPECT_EQ(metrics.last_mean_surface_deviation, 0.0);
        EXPECT_EQ(metrics.average_mean_surface_deviation, 0.0);
        EXPECT_EQ(metrics.last_rms_surface_deviation, 0.0);
        EXPECT_EQ(metrics.average_rms_surface_deviation, 0.0);
        EXPECT_EQ(metrics.last_surface_deviation_sample_count, 0U);
        EXPECT_EQ(metrics.total_surface_deviation_sample_count, 0U);
        EXPECT_TRUE(metrics.last_job_label.empty());
    }
} // namespace engine::geometry