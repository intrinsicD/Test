#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/api.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
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
    ASSERT_EQ(output.parameterization.charts.size(), 1U);
    EXPECT_GT(output.parameterization.charts.front().area, 0.0F);
    EXPECT_GT(output.parameterization.charts.front().scale, 0.0F);
    EXPECT_GT(output.parameterization.texel_density, 0.0F);
    EXPECT_GT(output.parameterization.total_chart_area, 0.0F);
    EXPECT_GT(output.parameterization.atlas_area, 0.0F);
    EXPECT_NEAR(output.parameterization.fill_ratio, 1.0F, 1e-3F);
    EXPECT_NEAR(output.parameterization.total_chart_area, output.parameterization.atlas_area, 1e-3F);
    EXPECT_NEAR(output.parameterization.total_seam_length, 8.0F, 1e-3F);
    EXPECT_NEAR(output.parameterization.charts.front().boundary_length, 8.0F, 1e-3F);
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
    ASSERT_EQ(output.parameterization.charts.size(), 1U);
    EXPECT_GT(output.parameterization.charts.front().area, 0.0F);
    EXPECT_GT(output.parameterization.charts.front().scale, 0.0F);
    EXPECT_GT(output.parameterization.texel_density, 0.0F);
    EXPECT_GT(output.parameterization.total_chart_area, 0.0F);
    EXPECT_GT(output.parameterization.atlas_area, 0.0F);
    EXPECT_NEAR(output.parameterization.fill_ratio, 1.0F, 1e-3F);
    EXPECT_NEAR(output.parameterization.total_chart_area, output.parameterization.atlas_area, 1e-3F);
    EXPECT_GT(output.parameterization.total_seam_length, 0.0F);
    EXPECT_GT(output.parameterization.charts.front().boundary_length, 0.0F);
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
    ASSERT_EQ(output.parameterization.charts.size(), 2U);
    EXPECT_FALSE(output.mesh.texture_coordinates.empty());
    EXPECT_GT(output.parameterization.total_seam_length, 0.0F);
    EXPECT_GT(output.parameterization.total_chart_area, 0.0F);
    EXPECT_GT(output.parameterization.atlas_area, 0.0F);
    EXPECT_GE(output.parameterization.fill_ratio, 0.0F);
    EXPECT_LE(output.parameterization.fill_ratio, 1.0F);
    float accumulated_boundary = 0.0F;
    for (const auto& chart : output.parameterization.charts)
    {
        EXPECT_GT(chart.boundary_length, 0.0F);
        accumulated_boundary += chart.boundary_length;
    }
    EXPECT_NEAR(accumulated_boundary, output.parameterization.total_seam_length, 1e-3F);
}

TEST(RemeshParameterizationTests, RepackExistingParameterizationNormalizesIslands)
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
    request.parameterization.repack_islands = true;
    request.parameterization.gutter_width = 0.2F;
    request.attribute_policy.texture_coordinates = geo::AttributeTransferMode::kPreserve;

    const auto result = geo::Remesh(request);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const geo::RemeshOutput& output = result.value();
    ASSERT_EQ(output.parameterization.charts.size(), 2U);
    EXPECT_GT(output.parameterization.total_seam_length, 0.0F);
    EXPECT_GT(output.parameterization.total_chart_area, 0.0F);
    EXPECT_GT(output.parameterization.atlas_area, 0.0F);
    EXPECT_GE(output.parameterization.fill_ratio, 0.0F);
    EXPECT_LE(output.parameterization.fill_ratio, 1.0F + 1e-4F);

    float min_u = std::numeric_limits<float>::max();
    float max_u = std::numeric_limits<float>::lowest();
    float min_v = std::numeric_limits<float>::max();
    float max_v = std::numeric_limits<float>::lowest();
    for (const auto& uv : output.mesh.texture_coordinates)
    {
        min_u = std::min(min_u, uv[0]);
        max_u = std::max(max_u, uv[0]);
        min_v = std::min(min_v, uv[1]);
        max_v = std::max(max_v, uv[1]);
    }

    EXPECT_GE(min_u, -1e-4F);
    EXPECT_GE(min_v, -1e-4F);
    EXPECT_LE(max_u, 1.0F + 1e-4F);
    EXPECT_LE(max_v, 1.0F + 1e-4F);

    const auto& charts = output.parameterization.charts;
    float seam_sum = 0.0F;
    bool translated = false;
    for (const auto& chart : charts)
    {
        EXPECT_GT(chart.boundary_length, 0.0F);
        seam_sum += chart.boundary_length;
        if (std::abs(chart.translation[0]) > 1e-4F || std::abs(chart.translation[1]) > 1e-4F)
        {
            translated = true;
        }
    }
    EXPECT_TRUE(translated);
    EXPECT_NEAR(seam_sum, output.parameterization.total_seam_length, 1e-3F);

    for (std::size_t i = 0; i < charts.size(); ++i)
    {
        for (std::size_t j = i + 1; j < charts.size(); ++j)
        {
            const float overlap_x = std::min(charts[i].max_uv[0], charts[j].max_uv[0]) -
                                    std::max(charts[i].min_uv[0], charts[j].min_uv[0]);
            const float overlap_y = std::min(charts[i].max_uv[1], charts[j].max_uv[1]) -
                                    std::max(charts[i].min_uv[1], charts[j].min_uv[1]);
            EXPECT_TRUE(overlap_x <= 1e-4F || overlap_y <= 1e-4F);
        }
    }
}

TEST(RemeshParameterizationTests, DisallowingReuseForcesRepack)
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
    request.parameterization.repack_islands = false;
    request.parameterization.allow_chart_reuse = false;
    request.parameterization.gutter_width = 0.0F;
    request.attribute_policy.texture_coordinates = geo::AttributeTransferMode::kPreserve;

    const auto result = geo::Remesh(request);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const geo::RemeshOutput& output = result.value();
    ASSERT_EQ(output.parameterization.charts.size(), 2U);
    EXPECT_GT(output.parameterization.total_seam_length, 0.0F);
    EXPECT_GT(output.parameterization.total_chart_area, 0.0F);
    EXPECT_GT(output.parameterization.atlas_area, 0.0F);
    EXPECT_GE(output.parameterization.fill_ratio, 0.0F);
    EXPECT_LE(output.parameterization.fill_ratio, 1.0F + 1e-4F);
    const auto& charts = output.parameterization.charts;
    float seam_sum = 0.0F;
    bool moved = false;
    for (const auto& chart : charts)
    {
        EXPECT_GE(chart.min_uv[0], -1e-4F);
        EXPECT_GE(chart.min_uv[1], -1e-4F);
        EXPECT_LE(chart.max_uv[0], 1.0F + 1e-4F);
        EXPECT_LE(chart.max_uv[1], 1.0F + 1e-4F);
        EXPECT_GT(chart.boundary_length, 0.0F);
        seam_sum += chart.boundary_length;
        if (std::abs(chart.translation[0]) > 1e-4F || std::abs(chart.translation[1]) > 1e-4F)
        {
            moved = true;
        }
    }
    EXPECT_TRUE(moved);
    EXPECT_NEAR(seam_sum, output.parameterization.total_seam_length, 1e-3F);
    const float translation_delta = std::abs(charts[0].translation[0] - charts[1].translation[0]) +
                                    std::abs(charts[0].translation[1] - charts[1].translation[1]);
    EXPECT_GT(translation_delta, 1e-4F);
}
