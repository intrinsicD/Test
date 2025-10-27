#include <gtest/gtest.h>

#include "engine/assets/mesh_asset.hpp"
#include "engine/assets/validation.hpp"
#include "engine/io/geometry_io.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    // Path to sample assets directory
    const std::filesystem::path SAMPLES_DIR = std::filesystem::path(__FILE__).parent_path().parent_path() / "samples";

    struct TempDirectory
    {
        TempDirectory()
        {
            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() /
                ("engine-handle-validation-" + std::to_string(timestamp));
            std::filesystem::create_directories(path);
        }

        ~TempDirectory()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        std::filesystem::path path{};
    };

    void write_text(const std::filesystem::path& file, std::string_view contents)
    {
        std::filesystem::create_directories(file.parent_path());
        std::ofstream stream{file};
        ASSERT_TRUE(stream.good());
        stream << contents;
    }

    const engine::assets::MeshAsset& load_test_mesh(engine::assets::MeshCache& cache,
                                                    engine::assets::MeshAssetDescriptor& descriptor,
                                                    const std::filesystem::path& base)
    {
        const auto path = base / "quad.obj";
        write_text(path,
                   "v 0 0 0\n"
                   "v 1 0 0\n"
                   "v 0 1 0\n"
                   "f 1 2 3\n");
        descriptor = engine::assets::MeshAssetDescriptor::from_file(
            path, engine::io::MeshFileFormat::obj);
        return cache.load(descriptor);
    }
} // namespace

TEST(HandleValidation, RecordsSuccessForValidMeshHandle)
{
    engine::assets::HandleValidationTelemetry::instance().reset();

    engine::assets::MeshCache cache;
    TempDirectory temp;
    engine::assets::MeshAssetDescriptor descriptor{};
    [[maybe_unused]] const auto& asset = load_test_mesh(cache, descriptor, temp.path);

    const auto result = engine::assets::validate_handle_status(descriptor.handle, "HandleValidation.ValidMesh");
    EXPECT_TRUE(result.valid);
    EXPECT_FALSE(result.failure.has_value());

    const auto snapshot = engine::assets::HandleValidationTelemetry::instance().snapshot();
    ASSERT_FALSE(snapshot.empty());
    const auto& entry = snapshot.front();
    EXPECT_EQ(entry.type, "MeshHandle");
    EXPECT_EQ(entry.success_count, 1U);
    EXPECT_EQ(entry.failure_count, 0U);
}

TEST(HandleValidation, DetectsUnboundMeshHandle)
{
    engine::assets::HandleValidationTelemetry::instance().reset();

    engine::assets::MeshCache cache;
    TempDirectory temp;
    engine::assets::MeshAssetDescriptor descriptor{};
    [[maybe_unused]] const auto& asset = load_test_mesh(cache, descriptor, temp.path);

    cache.unload(descriptor.handle);

    const auto result = engine::assets::validate_handle_status(descriptor.handle, "HandleValidation.UnboundMesh");
    ASSERT_FALSE(result.valid);
    ASSERT_TRUE(result.failure.has_value());
    EXPECT_EQ(result.failure->reason, "Handle is not bound");
    EXPECT_EQ(result.failure->context, "HandleValidation.UnboundMesh");

    const auto snapshot = engine::assets::HandleValidationTelemetry::instance().snapshot();
    ASSERT_FALSE(snapshot.empty());
    const auto& entry = snapshot.front();
    EXPECT_EQ(entry.failure_count, 1U);
    EXPECT_EQ(entry.last_failure_reason, "Handle is not bound");
}

TEST(HandleValidation, DetectsStaleMeshHandle)
{
    engine::assets::HandleValidationTelemetry::instance().reset();

    engine::assets::MeshCache cache;
    TempDirectory temp;
    engine::assets::MeshAssetDescriptor descriptor{};
    [[maybe_unused]] const auto& asset = load_test_mesh(cache, descriptor, temp.path);

    const auto raw = descriptor.handle.raw_handle();
    cache.unload(descriptor.handle);
    descriptor.handle.bind(raw);

    const auto result = engine::assets::validate_handle_status(descriptor.handle, "HandleValidation.StaleMesh");
    ASSERT_FALSE(result.valid);
    ASSERT_TRUE(result.failure.has_value());
    EXPECT_EQ(result.failure->reason, "Handle validator rejected handle");

    const auto snapshot = engine::assets::HandleValidationTelemetry::instance().snapshot();
    ASSERT_FALSE(snapshot.empty());
    const auto& entry = snapshot.front();
    EXPECT_EQ(entry.failure_count, 1U);
    EXPECT_EQ(entry.last_failure_context, "HandleValidation.StaleMesh");
    EXPECT_EQ(entry.last_failure_reason, "Handle validator rejected handle");
}