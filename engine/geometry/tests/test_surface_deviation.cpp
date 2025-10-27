#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"
#include "engine/geometry/remesh/deviation.hpp"

namespace engine::geometry
{
    namespace
    {
        SurfaceMesh make_single_triangle()
        {
            SurfaceMesh mesh{};
            mesh.positions = {
                {0.0F, 0.0F, 0.0F},
                {1.0F, 0.0F, 0.0F},
                {0.0F, 1.0F, 0.0F},
            };
            mesh.rest_positions = mesh.positions;
            mesh.indices = {0U, 1U, 2U};
            return mesh;
        }
    } // namespace

    TEST(SurfaceDeviationMetrics, ZeroForIdenticalMeshes)
    {
        SurfaceMesh mesh = make_single_triangle();
        const SurfaceDeviationMetrics metrics = ComputeSurfaceDeviationMetrics(mesh, mesh);

        EXPECT_NEAR(metrics.max_distance, 0.0F, 1e-6F);
        EXPECT_NEAR(metrics.mean_distance, 0.0F, 1e-6F);
        EXPECT_NEAR(metrics.rms_distance, 0.0F, 1e-6F);
        EXPECT_NEAR(metrics.reference_to_candidate_max, 0.0F, 1e-6F);
        EXPECT_NEAR(metrics.candidate_to_reference_max, 0.0F, 1e-6F);
        EXPECT_GT(metrics.sample_count, 0U);
    }

    TEST(SurfaceDeviationMetrics, DetectsUniformTranslation)
    {
        SurfaceMesh reference = make_single_triangle();
        SurfaceMesh translated = reference;
        apply_uniform_translation(translated, engine::math::vec3{0.0F, 0.0F, 0.1F});

        const SurfaceDeviationMetrics metrics = ComputeSurfaceDeviationMetrics(reference, translated);

        EXPECT_NEAR(metrics.max_distance, 0.1F, 1e-4F);
        EXPECT_NEAR(metrics.mean_distance, 0.1F, 1e-4F);
        EXPECT_NEAR(metrics.rms_distance, 0.1F, 1e-4F);
        EXPECT_NEAR(metrics.reference_to_candidate_max, 0.1F, 1e-4F);
        EXPECT_NEAR(metrics.candidate_to_reference_max, 0.1F, 1e-4F);
        EXPECT_GT(metrics.sample_count, 0U);
    }
} // namespace engine::geometry