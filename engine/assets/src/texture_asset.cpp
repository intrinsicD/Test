#include "engine/assets/texture_asset.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] std::filesystem::file_time_type safe_last_write_time(const std::filesystem::path& path)
        {
            std::error_code ec;
            const auto time = std::filesystem::last_write_time(path, ec);
            if (ec)
            {
                throw std::runtime_error("Failed to query texture asset timestamp: " + ec.message());
            }
            return time;
        }

        void read_binary(const std::filesystem::path& path, std::vector<std::byte>& output)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
            {
                throw std::runtime_error("Failed to open texture file: " + path.generic_string());
            }

            stream.seekg(0, std::ios::end);
            const auto size = static_cast<std::size_t>(stream.tellg());
            stream.seekg(0, std::ios::beg);
            output.resize(size);
            if (!stream.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(size)))
            {
                throw std::runtime_error("Failed to read texture file: " + path.generic_string());
            }
        }
    } // namespace

    const TextureAsset& TextureCache::load(const TextureAssetDescriptor& descriptor)
    {
        auto [it, inserted] = assets_.try_emplace(descriptor.handle);
        TextureAsset& asset = it->second;
        asset.descriptor = descriptor;

        const auto current_write = safe_last_write_time(descriptor.source);
        const bool needs_reload = inserted || asset.last_write != current_write;
        if (needs_reload)
        {
            reload_asset(descriptor.handle, asset, !inserted);
        }

        return asset;
    }

    bool TextureCache::contains(const TextureHandle& handle) const
    {
        return assets_.find(handle) != assets_.end();
    }

    const TextureAsset& TextureCache::get(const TextureHandle& handle) const
    {
        const auto it = assets_.find(handle);
        if (it == assets_.end())
        {
            throw std::out_of_range("Texture asset handle not found");
        }
        return it->second;
    }

    void TextureCache::unload(const TextureHandle& handle)
    {
        assets_.erase(handle);
        callbacks_.erase(handle);
    }

    void TextureCache::register_hot_reload_callback(const TextureHandle& handle, HotReloadCallback callback)
    {
        callbacks_[handle].push_back(std::move(callback));
    }

    void TextureCache::poll()
    {
        for (auto& [handle, asset] : assets_)
        {
            const auto current_write = safe_last_write_time(asset.descriptor.source);
            if (current_write != asset.last_write)
            {
                reload_asset(handle, asset, true);
            }
        }
    }

    void TextureCache::reload_asset(const TextureHandle& handle, TextureAsset& asset, bool notify)
    {
        read_binary(asset.descriptor.source, asset.data);
        asset.last_write = safe_last_write_time(asset.descriptor.source);

        if (notify)
        {
            const auto cb_it = callbacks_.find(handle);
            if (cb_it != callbacks_.end())
            {
                for (const auto& callback : cb_it->second)
                {
                    callback(asset);
                }
            }
        }
    }
} // namespace engine::assets

