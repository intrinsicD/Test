#pragma once

#include "engine/assets/handles.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

namespace engine::assets {

enum class TextureColorSpace : std::uint8_t {
    linear = 0,
    srgb
};

struct TextureAssetDescriptor {
    TextureHandle handle;
    std::filesystem::path source;
    TextureColorSpace color_space{TextureColorSpace::linear};

    [[nodiscard]] static TextureAssetDescriptor from_file(
        const std::filesystem::path& path,
        TextureColorSpace space = TextureColorSpace::linear)
    {
        return TextureAssetDescriptor{TextureHandle{path}, path, space};
    }
};

struct TextureAsset {
    TextureAssetDescriptor descriptor{};
    std::vector<std::byte> data{};
    std::filesystem::file_time_type last_write{};
};

class TextureCache {
public:
    using HotReloadCallback = std::function<void(const TextureAsset&)>;

    [[nodiscard]] const TextureAsset& load(const TextureAssetDescriptor& descriptor);
    [[nodiscard]] bool contains(const TextureHandle& handle) const;
    [[nodiscard]] const TextureAsset& get(const TextureHandle& handle) const;

    void unload(const TextureHandle& handle);
    void register_hot_reload_callback(const TextureHandle& handle, HotReloadCallback callback);
    void poll();

private:
    void reload_asset(const TextureHandle& handle, TextureAsset& asset, bool notify);

    std::unordered_map<TextureHandle, TextureAsset> assets_{};
    std::unordered_map<TextureHandle, std::vector<HotReloadCallback>> callbacks_{};
};

}  // namespace engine::assets

