#include <gtest/gtest.h>

#include "engine/geometry/mesh/halfedge_mesh.hpp"
#include "engine/io/geometry_io.hpp"
#include "engine/io/telemetry.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    std::filesystem::path make_temp_directory()
    {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        std::filesystem::path path =
            std::filesystem::temp_directory_path() / ("engine-io-telemetry-" + std::to_string(timestamp));
        std::filesystem::create_directories(path);
        return path;
    }

    void write_obj_triangle(const std::filesystem::path& path)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream{path};
        ASSERT_TRUE(stream.good());
        stream << "v 0 0 0\n";
        stream << "v 1 0 0\n";
        stream << "v 0 1 0\n";
        stream << "f 1 2 3\n";
    }
} // namespace

TEST(GeometryIoTelemetry, RecordsSuccessAndFailureCounts)
{
    auto& telemetry = engine::io::GeometryIoTelemetry::instance();
    telemetry.reset_for_testing();

    const auto temp_dir = make_temp_directory();
    const auto mesh_path = temp_dir / "triangle.obj";
    write_obj_triangle(mesh_path);

    engine::geometry::Mesh mesh{};
    ASSERT_TRUE(engine::io::read_mesh(mesh_path, mesh.interface));

    const auto missing_path = temp_dir / "missing.obj";
    const auto failure = engine::io::read_mesh(missing_path, mesh.interface);
    ASSERT_FALSE(failure);

    const auto snapshot = telemetry.snapshot();
    const auto& counters = snapshot.operation(engine::io::GeometryIoOperation::read_mesh);
    EXPECT_EQ(counters.attempts, 2U);
    EXPECT_EQ(counters.successes, 1U);

    const auto file_not_found_index =
        engine::io::geometry_io_error_index(engine::io::GeometryIoError::file_not_found);
    ASSERT_LT(file_not_found_index, counters.failures_by_error.size());
    EXPECT_EQ(counters.failures_by_error[file_not_found_index], 1U);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}
