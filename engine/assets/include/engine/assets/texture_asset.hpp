#pragma once

#include "engine/assets/async.hpp"
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

    struct TextureAssetDescriptor
    {
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

    struct TextureAsset
    {
        TextureAssetDescriptor descriptor{};
        std::vector<std::byte> data{};
        std::filesystem::file_time_type last_write{};
    };

    class TextureCache
    {
    public:
        TextureCache();

        using HotReloadCallback = std::function<void(const TextureAsset&)>;

        [[nodiscard]] const TextureAsset& load(const TextureAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const TextureHandle& handle) const;
        [[nodiscard]] const TextureAsset& get(const TextureHandle& handle) const;

        void unload(const TextureHandle& handle);
        void register_hot_reload_callback(const TextureHandle& handle, HotReloadCallback callback);
        void poll();

    private:
        using Pool = core::memory::ResourcePool<TextureAsset, TextureHandleTag>;
        using RawHandle = typename Pool::handle_type;
        using HandleHasher = typename Pool::handle_hasher;

        engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, TextureAsset& asset, bool notify);
        void register_watch_locked(const RawHandle& handle, TextureAsset& asset);
        void unregister_watch_locked(const RawHandle& handle);

        Pool assets_{};
        std::unordered_map<std::string, RawHandle> bindings_{};
        std::unordered_map<std::string, std::vector<HotReloadCallback>> pending_callbacks_{};
        std::unordered_map<RawHandle, std::vector<HotReloadCallback>, HandleHasher> callbacks_{};
        std::unordered_map<RawHandle, platform::filesystem::FilesystemWatcher::WatchHandle, HandleHasher> watch_handles_
            {};
        platform::filesystem::FilesystemWatcher watcher_{};
        mutable std::mutex mutex_{};
        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets