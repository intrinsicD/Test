#include <gtest/gtest.h>

#include "engine/assets/async.hpp"
#include "engine/assets/graph_asset.hpp"
#include "engine/assets/material_asset.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/assets/point_cloud_asset.hpp"
#include "engine/assets/shader_asset.hpp"
#include "engine/assets/texture_asset.hpp"

#include <array>
#include <cstddef>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <thread>
#include <string_view>
#include <system_error>
#include <vector>

#include "engine/core/memory/resource_pool.hpp"

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
                ("engine-assets-" + std::to_string(timestamp));
            std::filesystem::create_directories(path);
        }

        ~TempDirectory()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        std::filesystem::path path;
    };

    void write_text(const std::filesystem::path& path, std::string_view content)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream{path};
        ASSERT_TRUE(stream.good());
        stream << content;
    }

    void write_binary(const std::filesystem::path& path, std::span<const std::byte> content)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream{path, std::ios::binary};
        ASSERT_TRUE(stream.good());
        stream.write(reinterpret_cast<const char*>(content.data()),
                     static_cast<std::streamsize>(content.size()));
    }

    std::vector<std::byte> decode_base64(std::string_view encoded)
    {
        static const auto lut = []
        {
            std::array<int, 256> table{};
            table.fill(-1);
            for (int i = 0; i < 26; ++i)
            {
                table['A' + i] = i;
                table['a' + i] = i + 26;
            }
            for (int i = 0; i < 10; ++i)
            {
                table['0' + i] = i + 52;
            }
            table[static_cast<unsigned char>('+')] = 62;
            table[static_cast<unsigned char>('/')] = 63;
            return table;
        }();

        std::vector<std::byte> output{};
        output.reserve((encoded.size() / 4U) * 3U);
        int value = 0;
        int bits = 0;
        for (unsigned char c : encoded)
        {
            if (c == '=')
            {
                break;
            }
            const int decoded = lut[c];
            if (decoded < 0)
            {
                continue;
            }
            value = (value << 6) | decoded;
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                const unsigned char byte = static_cast<unsigned char>((value >> bits) & 0xFF);
                output.push_back(static_cast<std::byte>(byte));
            }
        }
        return output;
    }

    const std::vector<std::byte>& gradient_png()
    {
        static const std::vector<std::byte> data = decode_base64(
            "iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAYAAACp8Z5+AAAAKUlEQVR4nBXIMQEAMAzDsJAMn+Exycw99CjJW4WmpIbQeoEhNC5mCE0ftNQn4VOSi94AAAAASUVORK5CYII=");
        return data;
    }

    const std::vector<std::byte>& alternate_png()
    {
        static const std::vector<std::byte> data = decode_base64(
            "iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAYAAACp8Z5+AAAAM0lEQVR4nBXIQREAMQDCQPTUD3rqJ3rOT8oND2YTjXjsFu+vI4wMsNBK727ot5ArwWaIPoR/J9XDmnFHAAAAAElFTkSuQmCC");
        return data;
    }
} // namespace

TEST(MeshCache, LoadsMeshData)
{
    const auto path = SAMPLES_DIR / "triangle.obj";

    engine::assets::MeshCache cache;
    const auto descriptor = engine::assets::MeshAssetDescriptor::from_file(path, engine::io::MeshFileFormat::obj);
    const auto& asset = cache.load(descriptor);

    EXPECT_TRUE(descriptor.handle.is_bound());
    EXPECT_EQ(asset.mesh.interface.vertex_count(), 3U);
    EXPECT_EQ(asset.mesh.interface.face_count(), 1U);

    const auto& cached = cache.get(descriptor.handle);
    EXPECT_EQ(&asset, &cached);
}

TEST(AssetHandles, BindingPropagatesAcrossCopies)
{
    engine::assets::MeshHandle handle{std::string{"mesh/test"}};
    engine::assets::MeshHandle copy = handle;

    engine::core::memory::ResourcePool<int, engine::assets::MeshHandleTag> pool;
    auto [raw_handle, value] = pool.acquire(42);
    value = 42;

    handle.bind(raw_handle);
    EXPECT_TRUE(handle.is_bound());
    EXPECT_TRUE(copy.is_bound());
    EXPECT_TRUE(handle.is_valid(pool));
    EXPECT_TRUE(copy.is_valid(pool));

    pool.release(raw_handle);
    EXPECT_FALSE(handle.is_valid(pool));
    EXPECT_FALSE(copy.is_valid(pool));
}

TEST(MeshCache, HotReloadNotifies)
{
    TempDirectory temp;
    const auto path = temp.path / "quad.obj";
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 1 1 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n");

    engine::assets::MeshCache cache;
    const auto descriptor = engine::assets::MeshAssetDescriptor::from_file(path, engine::io::MeshFileFormat::obj);

    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::MeshAsset& updated)
                                       {
                                           reloaded = true;
                                           EXPECT_EQ(updated.mesh.interface.face_count(), 2U);
                                       });

    [[maybe_unused]] const auto& initial_asset = cache.load(descriptor);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 1 1 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n"
               "f 1 3 4\n");

    cache.poll();
    EXPECT_TRUE(reloaded);
}

TEST(MeshCache, HotReloadTelemetryRecordsSuccess)
{
    TempDirectory temp;
    const auto path = temp.path / "hot_reload_success.obj";
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 1 1 0\n"
               "f 1 2 3\n");

    auto& telemetry = engine::assets::AssetHotReloadTelemetry::instance();
    telemetry.reset_for_testing();

    engine::assets::MeshCache cache;
    const auto descriptor =
        engine::assets::MeshAssetDescriptor::from_file(path, engine::io::MeshFileFormat::obj);

    [[maybe_unused]] const auto& asset = cache.load(descriptor);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 1 1 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n"
               "f 1 3 4\n");

    cache.poll();

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.hot_reload_attempts, 1U);
    EXPECT_EQ(snapshot.failure_count, 0U);
    EXPECT_TRUE(snapshot.recent_failures.empty());

    telemetry.reset_for_testing();
}

TEST(MeshCache, UnloadStopsHotReload)
{
    TempDirectory temp;
    const auto path = temp.path / "stale_quad.obj";
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 1 1 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n");

    engine::assets::MeshCache cache;
    auto descriptor = engine::assets::MeshAssetDescriptor::from_file(path, engine::io::MeshFileFormat::obj);
    [[maybe_unused]] const auto& asset = cache.load(descriptor);

    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::MeshAsset&) { reloaded = true; });

    cache.unload(descriptor.handle);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 1 1 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n"
               "f 1 3 4\n");

    cache.poll();
    EXPECT_FALSE(reloaded);
}

TEST(MeshCache, HotReloadTelemetryRecordsFailure)
{
    TempDirectory temp;
    const auto path = temp.path / "hot_reload_failure.obj";
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n");

    auto& telemetry = engine::assets::AssetHotReloadTelemetry::instance();
    telemetry.reset_for_testing();

    engine::assets::MeshCache cache;
    const auto descriptor =
        engine::assets::MeshAssetDescriptor::from_file(path, engine::io::MeshFileFormat::obj);

    [[maybe_unused]] const auto& asset = cache.load(descriptor);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path, "not a valid obj file\n");

    cache.poll();

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.hot_reload_attempts, 1U);
    EXPECT_EQ(snapshot.failure_count, 1U);
    ASSERT_FALSE(snapshot.recent_failures.empty());
    EXPECT_EQ(snapshot.recent_failures.front().identifier, descriptor.handle.id());
    EXPECT_FALSE(snapshot.last_error.empty());
    EXPECT_FALSE(snapshot.error_hint.empty());

    telemetry.reset_for_testing();
}

TEST(MeshCache, UnloadInvalidatesHandle)
{
    TempDirectory temp;
    const auto path = temp.path / "triangle.obj";
    write_text(path,
               "v 0 0 0\n"
               "v 1 0 0\n"
               "v 0 1 0\n"
               "f 1 2 3\n");

    engine::assets::MeshCache cache;
    const auto descriptor = engine::assets::MeshAssetDescriptor::from_file(path, engine::io::MeshFileFormat::obj);
    [[maybe_unused]] const auto& asset = cache.load(descriptor);
    EXPECT_TRUE(cache.contains(descriptor.handle));

    cache.unload(descriptor.handle);
    EXPECT_FALSE(cache.contains(descriptor.handle));
#ifndef NDEBUG
    EXPECT_DEATH((void)cache.get(descriptor.handle), "Mesh asset handle not found");
#else
    EXPECT_THROW(cache.get(descriptor.handle), std::out_of_range);
#endif
}

TEST(PointCloudCache, LoadsPointCloudData)
{
    TempDirectory temp;
    const auto path = temp.path / "cloud.ply";
    write_text(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 3\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "0 1 0\n");

    engine::assets::PointCloudCache cache;
    const auto descriptor = engine::assets::PointCloudAssetDescriptor::from_file(
        path, engine::io::PointCloudFileFormat::ply);
    const auto& asset = cache.load(descriptor);

    EXPECT_EQ(asset.point_cloud.interface.vertex_count(), 3U);

    const auto& cached = cache.get(descriptor.handle);
    EXPECT_EQ(&asset, &cached);
}

TEST(PointCloudCache, HotReloadNotifies)
{
    TempDirectory temp;
    const auto path = temp.path / "cloud_reload.ply";
    write_text(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 2\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n");

    engine::assets::PointCloudCache cache;
    const auto descriptor = engine::assets::PointCloudAssetDescriptor::from_file(
        path, engine::io::PointCloudFileFormat::ply);

    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::PointCloudAsset& updated)
                                       {
                                           reloaded = true;
                                           EXPECT_EQ(updated.point_cloud.interface.vertex_count(), 3U);
                                       });

    [[maybe_unused]] const auto& initial_asset = cache.load(descriptor);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 3\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "0 1 0\n");

    cache.poll();
    EXPECT_TRUE(reloaded);
}

TEST(GraphCache, LoadsGraphData)
{
    TempDirectory temp;
    const auto path = temp.path / "graph.ply";
    write_text(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 3\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "element edge 2\n"
               "property int vertex1\n"
               "property int vertex2\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "0 1 0\n"
               "0 1\n"
               "1 2\n");

    engine::assets::GraphCache cache;
    const auto descriptor = engine::assets::GraphAssetDescriptor::from_file(
        path, engine::io::GraphFileFormat::ply);
    const auto& asset = cache.load(descriptor);

    EXPECT_EQ(asset.graph.interface.vertex_count(), 3U);
    EXPECT_EQ(asset.graph.interface.edge_count(), 2U);

    const auto& cached = cache.get(descriptor.handle);
    EXPECT_EQ(&asset, &cached);
}

TEST(GraphCache, HotReloadNotifies)
{
    TempDirectory temp;
    const auto path = temp.path / "graph_reload.ply";
    write_text(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 4\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "element edge 2\n"
               "property int vertex1\n"
               "property int vertex2\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "1 1 0\n"
               "0 1 0\n"
               "0 1\n"
               "1 2\n");

    engine::assets::GraphCache cache;
    const auto descriptor = engine::assets::GraphAssetDescriptor::from_file(
        path, engine::io::GraphFileFormat::ply);

    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::GraphAsset& updated)
                                       {
                                           reloaded = true;
                                           EXPECT_EQ(updated.graph.interface.edge_count(), 3U);
                                       });

    [[maybe_unused]] const auto& initial_asset = cache.load(descriptor);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path,
               "ply\n"
               "format ascii 1.0\n"
               "element vertex 4\n"
               "property float x\n"
               "property float y\n"
               "property float z\n"
               "element edge 3\n"
               "property int vertex1\n"
               "property int vertex2\n"
               "end_header\n"
               "0 0 0\n"
               "1 0 0\n"
               "1 1 0\n"
               "0 1 0\n"
               "0 1\n"
               "1 2\n"
               "2 3\n");

    cache.poll();
    EXPECT_TRUE(reloaded);
}

TEST(TextureCache, LoadsTexturesWithMetadata)
{
    TempDirectory temp;
    const auto path = temp.path / "texture.png";
    write_binary(path, std::span{gradient_png()});

    engine::assets::TextureCache cache;
    engine::assets::TextureLoadingOptions options{};
    options.generate_mipmaps = true;
    options.max_mip_levels = 0;
    const auto descriptor = engine::assets::TextureAssetDescriptor::from_file(
        path, engine::assets::TextureColorSpace::srgb, options);

    const auto& asset = cache.load(descriptor);
    EXPECT_EQ(asset.format, engine::assets::TextureFormat::rgba8_unorm);
    EXPECT_EQ(asset.dimensions.width, 4U);
    EXPECT_EQ(asset.dimensions.height, 4U);
    ASSERT_EQ(asset.mip_levels.size(), 3U);

    const auto& base_level = asset.mip_levels.front();
    EXPECT_EQ(base_level.extent.width, 4U);
    EXPECT_EQ(base_level.extent.height, 4U);
    ASSERT_EQ(base_level.texels.size(), 4U * 4U * engine::assets::texture_bytes_per_pixel(
        engine::assets::TextureFormat::rgba8_unorm));

    const auto* base_pixels = reinterpret_cast<const unsigned char*>(base_level.texels.data());
    EXPECT_EQ(base_pixels[0], 0U);
    EXPECT_EQ(base_pixels[1], 0U);
    EXPECT_EQ(base_pixels[2], 128U);
    EXPECT_EQ(base_pixels[3], 255U);

    EXPECT_EQ(asset.encoded_payload.size(), gradient_png().size());

    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::TextureAsset& updated)
                                       {
                                           reloaded = true;
                                           EXPECT_EQ(updated.format, engine::assets::TextureFormat::rgba8_unorm);
                                           EXPECT_FALSE(updated.mip_levels.empty());
                                       });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    write_binary(path, std::span{alternate_png()});

    const auto& updated = cache.load(descriptor);
    EXPECT_TRUE(reloaded);
    EXPECT_EQ(updated.encoded_payload.size(), alternate_png().size());
    ASSERT_FALSE(updated.mip_levels.empty());
    const auto& last_level = updated.mip_levels.back();
    EXPECT_EQ(last_level.extent.width, 1U);
    EXPECT_EQ(last_level.extent.height, 1U);
}

TEST(TextureFormatUtilities, ComputesProperties)
{
    using namespace engine::assets;
    EXPECT_EQ(texture_channel_count(TextureFormat::rgba8_unorm), 4U);
    EXPECT_EQ(texture_bytes_per_pixel(TextureFormat::rgba32_float), 16U);
    EXPECT_EQ(compute_max_mip_levels(TextureDimensions{4U, 4U, 1U}), 3U);
}

TEST(ShaderCache, CompilesAndHotReloads)
{
    TempDirectory temp;
    const auto path = temp.path / "shader.vert";
    write_text(path, "void main() {}\n");

    engine::assets::ShaderCache cache;
    const auto descriptor = engine::assets::ShaderAssetDescriptor::from_file(path, engine::assets::ShaderStage::vertex);

    const auto& asset = cache.load(descriptor);
    EXPECT_FALSE(asset.source.empty());
    EXPECT_FALSE(asset.binary.spirv.empty());

    std::size_t previous_size = asset.binary.spirv.size();
    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::ShaderAsset& updated)
                                       {
                                           reloaded = true;
                                           EXPECT_GE(updated.binary.spirv.size(), previous_size);
                                       });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    write_text(path, "// comment\nvoid main() { gl_Position = vec4(0.0); }\n");

    cache.poll();
    EXPECT_TRUE(reloaded);
}

TEST(MaterialCache, StoresDescriptors)
{
    engine::assets::MaterialCache cache;
    const engine::assets::MaterialHandle material_handle{std::string{"material/basic"}};
    const engine::assets::ShaderHandle vs{std::string{"shader/vs"}};
    const engine::assets::ShaderHandle fs{std::string{"shader/fs"}};
    const engine::assets::TextureHandle tex{std::string{"texture/diffuse"}};

    const auto descriptor = engine::assets::MaterialAssetDescriptor::from_handles(
        material_handle, "Basic", vs, fs, std::vector<engine::assets::TextureHandle>{tex});

    const auto& asset = cache.load(descriptor);
    EXPECT_EQ(asset.descriptor.name, "Basic");
    ASSERT_EQ(asset.descriptor.textures.size(), 1U);
    EXPECT_EQ(asset.descriptor.textures.front().id(), tex.id());
}

TEST(AssetHotReloadTelemetry, RetainsRecentFailures)
{
    using engine::assets::AssetHotReloadTelemetry;
    using engine::assets::AssetLoadErrorCategory;

    auto& telemetry = AssetHotReloadTelemetry::instance();
    telemetry.reset_for_testing();

    for (int index = 0; index < 10; ++index)
    {
        const std::string identifier = "material/" + std::to_string(index);
        const auto error = engine::assets::make_asset_load_error(
            AssetLoadErrorCategory::ValidationError,
            "Validation failed for material " + std::to_string(index));
        telemetry.record_failure(error, identifier);
    }

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.failure_count, 10U);
    ASSERT_EQ(snapshot.recent_failures.size(), 8U);
    EXPECT_EQ(snapshot.recent_failures.front().identifier, "material/9");
    EXPECT_EQ(snapshot.recent_failures.front().error,
              "Validation failed for material 9");
    EXPECT_EQ(snapshot.recent_failures.front().hint,
              "Check asset metadata, dependencies, and descriptor configuration.");
    EXPECT_EQ(snapshot.recent_failures.back().identifier, "material/2");

    telemetry.reset_for_testing();
}