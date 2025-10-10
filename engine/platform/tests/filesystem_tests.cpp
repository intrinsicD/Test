#include <gtest/gtest.h>

#include "engine/platform/filesystem/filesystem.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using namespace engine::platform::filesystem;

class TempDirectory {
public:
    TempDirectory() {
        const auto base = std::filesystem::temp_directory_path();
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = base / ("engine_platform_fs_" + std::to_string(timestamp));
        std::filesystem::create_directories(path_);
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    TempDirectory(TempDirectory&&) = delete;
    TempDirectory& operator=(TempDirectory&&) = delete;

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    [[nodiscard]] std::filesystem::path path() const {
        return path_;
    }

private:
    std::filesystem::path path_{};
};

void write_text_file(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream stream(path, std::ios::out | std::ios::binary);
    stream << contents;
}

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& data) {
    std::ofstream stream(path, std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

TEST(Filesystem, ProvidesSandboxedAccess) {
    TempDirectory directory;
    const auto root = directory.path();
    const auto file_path = root / "config.txt";
    write_text_file(file_path, "content");

    Filesystem fs{root};
    EXPECT_EQ(fs.root(), std::filesystem::absolute(root).lexically_normal());
    EXPECT_TRUE(fs.exists("config.txt"));
    EXPECT_FALSE(fs.exists("../config.txt"));
    EXPECT_TRUE(fs.is_file("config.txt"));
    EXPECT_FALSE(fs.is_directory("config.txt"));

    const auto text = fs.read_text("config.txt");
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(*text, "content");

    EXPECT_FALSE(fs.read_text("../config.txt").has_value());
}

TEST(Filesystem, ReadsBinaryPayloads) {
    TempDirectory directory;
    const auto root = directory.path();
    const auto file_path = root / "blob.bin";
    const std::vector<std::byte> payload = {
        std::byte{0x01},
        std::byte{0x7F},
        std::byte{0x10},
        std::byte{0xFF},
    };
    write_binary_file(file_path, payload);

    Filesystem fs{root};
    const auto data = fs.read_binary("blob.bin");
    ASSERT_TRUE(data.has_value());
    ASSERT_EQ(data->size(), payload.size());
    EXPECT_TRUE(std::equal(data->begin(), data->end(), payload.begin(), payload.end()));

    EXPECT_FALSE(fs.read_binary("missing.bin").has_value());
}

TEST(VirtualFilesystem, RoutesRequestsToMountedProviders) {
    TempDirectory assets_dir;
    const auto root = assets_dir.path();
    write_text_file(root / "shader.glsl", "void main() {}");

    VirtualFilesystem vfs;
    EXPECT_TRUE(vfs.mount("assets", Filesystem{root}));
    EXPECT_TRUE(vfs.is_mounted("assets"));
    EXPECT_TRUE(vfs.exists("assets:/shader.glsl"));

    const auto text = vfs.read_text("assets:/shader.glsl");
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(*text, "void main() {}");

    EXPECT_FALSE(vfs.exists("assets:shader.glsl"));
    EXPECT_FALSE(vfs.exists("textures:/shader.glsl"));

    EXPECT_TRUE(vfs.unmount("assets"));
    EXPECT_FALSE(vfs.is_mounted("assets"));
}

TEST(VirtualFilesystem, RejectsInvalidMountsAndPaths) {
    VirtualFilesystem vfs;
    EXPECT_FALSE(vfs.mount("", Filesystem{std::filesystem::current_path()}));
    EXPECT_FALSE(vfs.exists(":/asset.txt"));
    EXPECT_FALSE(vfs.exists("assets://asset.txt"));
    EXPECT_FALSE(vfs.exists("assets:"));
}

}  // namespace

