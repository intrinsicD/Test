#include <gtest/gtest.h>

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
#include <thread>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{
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
} // namespace

TEST(MeshCache, LoadsMeshData)
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
    const auto& asset = cache.load(descriptor);

    EXPECT_EQ(asset.mesh.interface.vertex_count(), 3U);
    EXPECT_EQ(asset.mesh.interface.face_count(), 1U);

    const auto& cached = cache.get(descriptor.handle);
    EXPECT_EQ(&asset, &cached);
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
                                       [&](const engine::assets::MeshAsset& updated) {
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
                                       [&](const engine::assets::PointCloudAsset& updated) {
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
                                       [&](const engine::assets::GraphAsset& updated) {
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

TEST(TextureCache, ProvidesBinaryPayload)
{
    TempDirectory temp;
    const auto path = temp.path / "texture.bin";
    const std::array<std::byte, 4> payload{std::byte{0x00}, std::byte{0xFF}, std::byte{0x80}, std::byte{0x40}};
    write_binary(path, payload);

    engine::assets::TextureCache cache;
    const auto descriptor = engine::assets::TextureAssetDescriptor::from_file(path);

    const auto& asset = cache.load(descriptor);
    ASSERT_EQ(asset.data.size(), payload.size());
    EXPECT_EQ(std::to_integer<unsigned char>(asset.data[1]),
              std::to_integer<unsigned char>(payload[1]));

    bool reloaded = false;
    cache.register_hot_reload_callback(descriptor.handle,
                                       [&](const engine::assets::TextureAsset& updated) {
                                           reloaded = true;
                                           EXPECT_GT(updated.data.size(), payload.size());
                                       });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const std::array<std::byte, 6> new_payload{std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
                                               std::byte{0x04}, std::byte{0x05}, std::byte{0x06}};
    write_binary(path, new_payload);

    cache.poll();
    EXPECT_TRUE(reloaded);
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
                                       [&](const engine::assets::ShaderAsset& updated) {
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

