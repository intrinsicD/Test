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

