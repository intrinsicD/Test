#include "engine/geometry/topology/surface_curvature.hpp"

#include <cmath>

#include "engine/geometry/api.hpp"
#include "engine/math/common.hpp"

#include <gtest/gtest.h>

namespace engine::geometry
{
    namespace
    {
        [[nodiscard]] SurfaceMesh make_regular_tetrahedron()
        {
            SurfaceMesh mesh{};
            mesh.positions = {
                math::vec3{0.0F, 0.0F, 0.0F},
                math::vec3{1.0F, 0.0F, 0.0F},
                math::vec3{0.5F, std::sqrt(3.0F) / 2.0F, 0.0F},
                math::vec3{0.5F, std::sqrt(3.0F) / 6.0F, std::sqrt(2.0F / 3.0F)},
            };
            mesh.indices = {
                0, 1, 2,
                0, 3, 1,
                0, 2, 3,
                1, 3, 2,
            };
            return mesh;
        }
    } // namespace

    TEST(SurfaceCurvature, EmptyMeshProducesEmptyResults)
    {
        const SurfaceMesh mesh{};
        const SurfaceCurvatureResult result = ComputeSurfaceCurvature(mesh);

        EXPECT_TRUE(result.mean_curvature.empty());
        EXPECT_TRUE(result.gaussian_curvature.empty());
    }

    TEST(SurfaceCurvature, RegularTetrahedronCurvatureMatchesAnalyticBaseline)
    {
        const SurfaceMesh mesh = make_regular_tetrahedron();
        const SurfaceCurvatureResult result = ComputeSurfaceCurvature(mesh);

        ASSERT_EQ(result.mean_curvature.size(), mesh.positions.size());
        ASSERT_EQ(result.gaussian_curvature.size(), mesh.positions.size());

        const float face_area = std::sqrt(3.0F) / 4.0F;
        const float vertex_area = face_area;
        const float gaussian_expected = std::numbers::pi_v<float> / vertex_area;
        const float mean_expected = std::numbers::sqrt2_v<float> / vertex_area;

        for (std::size_t index = 0; index < mesh.positions.size(); ++index)
        {
            EXPECT_NEAR(result.gaussian_curvature[index], gaussian_expected, 1e-4F);
            EXPECT_NEAR(result.mean_curvature[index], mean_expected, 1e-4F);
        }
    }

    TEST(SurfaceCurvature, BoundaryVerticesReportZeroGaussianCurvature)
    {
        SurfaceMesh mesh = make_unit_quad();
        const SurfaceCurvatureResult result = ComputeSurfaceCurvature(mesh);

        ASSERT_EQ(result.gaussian_curvature.size(), mesh.positions.size());
        ASSERT_EQ(result.mean_curvature.size(), mesh.positions.size());

        for (std::size_t index = 0; index < mesh.positions.size(); ++index)
        {
            EXPECT_FLOAT_EQ(result.gaussian_curvature[index], 0.0F);
        }
    }
} // namespace engine::geometry
