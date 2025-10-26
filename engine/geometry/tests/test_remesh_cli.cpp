#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"
#include "remesh_cli.hpp"
#include "engine/platform/filesystem/filesystem.hpp"

#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>

namespace geo = engine::geometry;
namespace tools = engine::geometry::tools;
namespace fs = engine::platform::filesystem;

namespace
{
    class DirectoryGuard
    {
    public:
        explicit DirectoryGuard(std::filesystem::path directory)
            : directory_{std::move(directory)}
        {
        }

        DirectoryGuard(const DirectoryGuard&) = delete;
        DirectoryGuard& operator=(const DirectoryGuard&) = delete;

        ~DirectoryGuard()
        {
            if (directory_.empty())
            {
                return;
            }

            std::error_code ec;
            std::filesystem::remove_all(directory_, ec);
            (void)ec;
        }

        [[nodiscard]] const std::filesystem::path& path() const noexcept
        {
            return directory_;
        }

    private:
        std::filesystem::path directory_{};
    };
}

TEST(RemeshCliParsing, RequiresInputPath)
{
    const std::array<const char*, 1> argv = {"geometry_remesh"};
    const auto result = tools::ParseArguments(std::span(argv));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Missing required --input path");
}

TEST(RemeshCliParsing, ParsesFeatureModeConfiguration)
{
    const std::array<const char*, 11> argv = {
        "geometry_remesh",
        "--input",
        "mesh.obj",
        "--output",
        "out.obj",
        "--mode",
        "feature",
        "--target-edge-length",
        "0.125",
        "--parameterization",
        "lscm",
    };

    const auto result = tools::ParseArguments(std::span(argv));
    ASSERT_TRUE(result.has_value()) << result.error();

    const tools::RemeshCliOptions& options = result.value();
    EXPECT_EQ(options.input_path, std::filesystem::path{"mesh.obj"});
    EXPECT_EQ(options.output_path, std::filesystem::path{"out.obj"});
    EXPECT_EQ(options.mode, geo::RemeshingMode::kFeaturePreserving);
    ASSERT_TRUE(options.targets.target_edge_length.has_value());
    EXPECT_FLOAT_EQ(options.targets.target_edge_length.value(), 0.125F);
    EXPECT_EQ(options.parameterization.mode, geo::ParameterizationMode::kGenerateLscm);
}

TEST(RemeshCliExecution, ProducesRemeshedMesh)
{
    DirectoryGuard guard{
        std::filesystem::temp_directory_path() / ("geometry-remesh-cli-" + fs::generate_random_suffix())};
    std::filesystem::create_directories(guard.path());

    const std::filesystem::path input_path = guard.path() / "input.obj";
    const std::filesystem::path output_path = guard.path() / "output.obj";

    geo::SurfaceMesh mesh = geo::make_unit_quad();
    geo::save_surface_mesh(mesh, input_path);

    tools::RemeshCliOptions options{};
    options.input_path = input_path;
    options.output_path = output_path;
    options.mode = geo::RemeshingMode::kUniform;
    options.targets.relative_edge_scale = 0.75F;
    options.max_iterations = 6U;
    options.relaxation_factor = 0.6F;
    options.tangential_smoothing_weight = 0.0F;
    options.record_diagnostics = false;

    const auto execution = tools::ExecuteRemesh(options);
    ASSERT_TRUE(execution.has_value()) << execution.error();
    EXPECT_TRUE(std::filesystem::exists(output_path));

    const geo::SurfaceMesh remeshed = geo::load_surface_mesh(output_path);
    EXPECT_FALSE(remeshed.positions.empty());
    EXPECT_EQ(remeshed.indices.size() % 3U, 0U);
}

TEST(RemeshCliSummary, EmitsDatasetManifestEntry)
{
    tools::RemeshCliOptions options{};
    options.input_path = std::filesystem::path{"input.obj"};
    options.output_path = std::filesystem::path{"output.obj"};
    options.mode = geo::RemeshingMode::kUniform;
    options.targets.target_edge_length = 0.25F;
    options.targets.maximum_normal_deviation_degrees = 12.5F;
    options.parameterization.mode = geo::ParameterizationMode::kGenerateLscm;
    options.parameterization.target_texel_density = 256.0F;
    options.parameterization.gutter_width = 4.0F;
    options.parameterization.repack_islands = true;
    options.parameterization.allow_chart_reuse = false;
    options.job_label = std::string{"Remesh Sample"};

    tools::RemeshCliExecutionResult result{};
    result.input_vertex_count = 4U;
    result.input_face_count = 2U;
    result.output.mesh = geo::make_unit_quad();
    result.input_edge_statistics = geo::ComputeMeshEdgeStatistics(result.output.mesh);

    const geo::MeshEdgeStatistics output_edges = geo::ComputeMeshEdgeStatistics(result.output.mesh);
    result.output.statistics.iteration_count = 8U;
    result.output.statistics.min_edge_length = output_edges.min_edge_length;
    result.output.statistics.max_edge_length = output_edges.max_edge_length;
    result.output.statistics.max_error = 0.0025F;
    result.output.statistics.max_surface_deviation = 0.015F;
    result.output.statistics.mean_surface_deviation = 0.010F;
    result.output.statistics.rms_surface_deviation = 0.012F;

    geo::ParameterizationSummary parameterization{};
    parameterization.chart_count = 1U;
    parameterization.texel_density = 256.0F;
    parameterization.average_stretch = 1.015F;
    parameterization.max_stretch = 1.125F;
    parameterization.fill_ratio = 0.875F;
    parameterization.total_seam_length = 4.0F;
    parameterization.atlas_area = 1.0F;
    parameterization.total_chart_area = 0.88F;
    geo::ParameterizationChart chart{};
    chart.min_uv = engine::math::vec2{0.0F, 0.0F};
    chart.max_uv = engine::math::vec2{1.0F, 1.0F};
    chart.translation = engine::math::vec2{0.0F, 0.0F};
    chart.scale = 1.0F;
    chart.area = 1.0F;
    chart.boundary_length = 4.0F;
    parameterization.charts.push_back(chart);
    result.output.parameterization = parameterization;

    const std::string manifest = tools::BuildDatasetManifestEntry(options, result);

    EXPECT_NE(manifest.find("datasets:"), std::string::npos);
    EXPECT_NE(manifest.find("schema:\n      id: ai-004.dataset"), std::string::npos);
    EXPECT_NE(manifest.find("version: 1"), std::string::npos);
    EXPECT_NE(manifest.find("id: remesh-sample"), std::string::npos);
    EXPECT_NE(manifest.find("job_label: \"Remesh Sample\""), std::string::npos);
    EXPECT_NE(manifest.find("generator: geometry_remesh"), std::string::npos);
    EXPECT_NE(manifest.find("mode: uniform"), std::string::npos);
    EXPECT_NE(manifest.find("target_edge_length: 0.2500"), std::string::npos);
    EXPECT_NE(manifest.find("texel_density: 256.0000"), std::string::npos);
    EXPECT_NE(manifest.find("chart_count: 1"), std::string::npos);
    EXPECT_NE(manifest.find("iterations: 8"), std::string::npos);
    EXPECT_NE(manifest.find("max_error: 0.0025"), std::string::npos);
    EXPECT_NE(manifest.find("max_surface_deviation: 0.0150"), std::string::npos);
    EXPECT_NE(manifest.find("mean_surface_deviation: 0.0100"), std::string::npos);
    EXPECT_NE(manifest.find("rms_surface_deviation: 0.0120"), std::string::npos);
}

