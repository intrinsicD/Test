#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/api.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <vector>

namespace geo = engine::geometry;

namespace
{
    [[nodiscard]] float range(const std::vector<engine::math::vec2>& values, std::size_t axis) noexcept
    {
        float min_value = std::numeric_limits<float>::max();
        float max_value = std::numeric_limits<float>::lowest();

        for (const auto& value : values)
        {
            min_value = std::min(min_value, value[axis]);
            max_value = std::max(max_value, value[axis]);
        }

        return max_value - min_value;
    }
} // namespace

TEST(RemeshParameterizationTests, GeneratesLscmParameterizationForQuad)
{
    geo::SurfaceMesh mesh = geo::make_unit_quad();
    mesh.texture_coordinates.clear();

    geo::RemeshRequest request{};
    request.input_mesh = &mesh;
    request.mode = geo::RemeshingMode::kUniform;
    request.targets.target_edge_length = 1.0F;
    request.attribute_policy.texture_coordinates = geo::AttributeTransferMode::kDrop;
    request.parameterization.mode = geo::ParameterizationMode::kGenerateLscm;
    request.parameterization.target_texel_density = 2.0F;

    const auto result = geo::Remesh(request);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const geo::RemeshOutput& output = result.value();
    ASSERT_EQ(output.mesh.texture_coordinates.size(), output.mesh.positions.size());
    EXPECT_EQ(output.parameterization.chart_count, 1U);
    EXPECT_GT(output.parameterization.texel_density, 0.0F);
    EXPECT_GT(range(output.mesh.texture_coordinates, 0U), 0.5F);
    EXPECT_GE(range(output.mesh.texture_coordinates, 1U), 0.0F);
    EXPECT_NEAR(output.parameterization.texel_density,
                request.parameterization.target_texel_density,
                0.05F);
}

TEST(RemeshParameterizationTests, GeneratesAbfppParameterizationForQuad)
{
    geo::SurfaceMesh mesh = geo::make_unit_quad();
    mesh.texture_coordinates.clear();

    geo::RemeshRequest request{};
    request.input_mesh = &mesh;
    request.mode = geo::RemeshingMode::kUniform;
    request.targets.target_edge_length = 1.0F;
    request.attribute_policy.texture_coordinates = geo::AttributeTransferMode::kDrop;
    request.parameterization.mode = geo::ParameterizationMode::kGenerateAbfpp;
    request.parameterization.target_texel_density = 2.0F;

    const auto result = geo::Remesh(request);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const geo::RemeshOutput& output = result.value();
    ASSERT_EQ(output.mesh.texture_coordinates.size(), output.mesh.positions.size());
    EXPECT_EQ(output.parameterization.chart_count, 1U);
    EXPECT_GT(output.parameterization.texel_density, 0.0F);
    EXPECT_GT(range(output.mesh.texture_coordinates, 0U), 0.5F);
    EXPECT_GT(range(output.mesh.texture_coordinates, 1U), 0.05F);
    EXPECT_NEAR(output.parameterization.texel_density,
                request.parameterization.target_texel_density,
                0.05F);
}

TEST(RemeshParameterizationTests, ReuseExistingParameterizationCountsMultipleCharts)
{
    geo::SurfaceMesh mesh{};
    mesh.positions = {
        engine::math::vec3{0.0F, 0.0F, 0.0F},
        engine::math::vec3{1.0F, 0.0F, 0.0F},
        engine::math::vec3{1.0F, 0.0F, 1.0F},
        engine::math::vec3{0.0F, 0.0F, 1.0F},
        engine::math::vec3{0.0F, 0.0F, 3.0F},
        engine::math::vec3{1.0F, 0.0F, 3.0F},
        engine::math::vec3{1.0F, 0.0F, 4.0F},
        engine::math::vec3{0.0F, 0.0F, 4.0F},
    };
    mesh.rest_positions = mesh.positions;
    mesh.indices = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
    mesh.normals.assign(mesh.positions.size(), engine::math::vec3{0.0F, 1.0F, 0.0F});
    mesh.texture_coordinates = {
        engine::math::vec2{0.0F, 0.0F},
        engine::math::vec2{1.0F, 0.0F},
        engine::math::vec2{1.0F, 1.0F},
        engine::math::vec2{0.0F, 1.0F},
        engine::math::vec2{2.0F, 0.0F},
        engine::math::vec2{3.0F, 0.0F},
        engine::math::vec2{3.0F, 1.0F},
        engine::math::vec2{2.0F, 1.0F},
    };
    geo::update_bounds(mesh);

    geo::RemeshRequest request{};
    request.input_mesh = &mesh;
    request.mode = geo::RemeshingMode::kUniform;
    request.targets.target_edge_length = 1.0F;
    request.parameterization.mode = geo::ParameterizationMode::kReuseExisting;
    request.attribute_policy.texture_coordinates = geo::AttributeTransferMode::kPreserve;

    const auto result = geo::Remesh(request);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const geo::RemeshOutput& output = result.value();
    EXPECT_EQ(output.parameterization.chart_count, 2U);
    EXPECT_FALSE(output.mesh.texture_coordinates.empty());
}
