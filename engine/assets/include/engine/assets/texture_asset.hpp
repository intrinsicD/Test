#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/detail/cache_common.hpp"
#include "engine/assets/handles.hpp"

#include "engine/core/memory/resource_pool.hpp"
#include "engine/platform/filesystem/watcher.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine::assets
{
    enum class TextureColorSpace : std::uint8_t
    {
        linear = 0,
        srgb
    };

    enum class TextureFormat : std::uint8_t
    {
        unknown = 0,
        rgba8_unorm,
        rgba32_float,
    };

    struct TextureDimensions
    {
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::uint32_t depth{1};
    };

    struct TextureMipLevel
    {
        TextureDimensions extent{};
        std::vector<std::byte> texels{};
    };

    struct TextureLoadingOptions
    {
        bool generate_mipmaps{true};
        std::uint32_t max_mip_levels{0};
        bool retain_encoded_payload{true};
    };

    struct TextureAssetDescriptor
    {
        TextureHandle handle{};
        std::filesystem::path source{};
        TextureColorSpace color_space{TextureColorSpace::linear};
        TextureLoadingOptions options{};

        [[nodiscard]] static TextureAssetDescriptor from_file(
            const std::filesystem::path& path,
            TextureColorSpace space = TextureColorSpace::linear,
            TextureLoadingOptions options = {})
        {
            return TextureAssetDescriptor{TextureHandle{path}, path, space, options};
        }
    };

    struct TextureAsset
    {
        TextureAssetDescriptor descriptor{};
        TextureFormat format{TextureFormat::unknown};
        TextureDimensions dimensions{};
        std::vector<TextureMipLevel> mip_levels{};
        std::vector<std::byte> encoded_payload{};
        std::filesystem::file_time_type last_write{};
    };

    [[nodiscard]] std::uint32_t texture_channel_count(TextureFormat format) noexcept;
    [[nodiscard]] std::uint32_t texture_bytes_per_pixel(TextureFormat format) noexcept;
    [[nodiscard]] std::uint32_t compute_max_mip_levels(TextureDimensions extent) noexcept;

    class TextureCache : public detail::AssetCacheLifecycle<
                             TextureCache,
                             TextureAsset,
                             TextureAssetDescriptor,
                             TextureHandle,
                             TextureHandleTag,
                             std::function<void(const TextureAsset&)>,
                             true>
    {
    public:
        using Base = detail::AssetCacheLifecycle<
            TextureCache,
            TextureAsset,
            TextureAssetDescriptor,
            TextureHandle,
            TextureHandleTag,
            std::function<void(const TextureAsset&)>,
            true>;

        TextureCache();

        using HotReloadCallback = std::function<void(const TextureAsset&)>;

        [[nodiscard]] const TextureAsset& load(const TextureAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const TextureHandle& handle) const;
        [[nodiscard]] const TextureAsset& get(const TextureHandle& handle) const;

        void unload(const TextureHandle& handle);
        void register_hot_reload_callback(const TextureHandle& handle, HotReloadCallback callback);
        void poll();

        using typename Base::RawHandle;

    protected:
        friend Base;
        engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, TextureAsset& asset, bool notify);

    private:
        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets
