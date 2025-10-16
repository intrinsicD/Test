#include "engine/assets/texture_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace engine::assets {

namespace {

void read_binary(const std::filesystem::path& path, std::vector<std::byte>& output)
{
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        throw std::runtime_error("Failed to open texture file: " + path.generic_string());
    }

    stream.seekg(0, std::ios::end);
    const auto size = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    output.resize(size);
    if (!stream.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(size))) {
        throw std::runtime_error("Failed to read texture file: " + path.generic_string());
    }
}

}  // namespace

const TextureAsset& TextureCache::load(const TextureAssetDescriptor& descriptor)
{
    const auto identifier = descriptor.handle.id();
    if (identifier.empty()) {
        throw std::invalid_argument("Texture handle identifier cannot be empty");
    }

    TextureAsset* asset = nullptr;
    RawHandle handle{};
    bool inserted = false;

    const auto lookup = bindings_.find(identifier);
    if (lookup == bindings_.end()) {
        auto [acquired_handle, slot] = assets_.acquire();
        handle = acquired_handle;
        asset = &slot;
        bindings_.emplace(identifier, handle);
        inserted = true;
    } else {
        handle = lookup->second;
        asset = &assets_.get(handle);
    }

    asset->descriptor = descriptor;
    descriptor.handle.bind(handle);

    if (auto pending = pending_callbacks_.find(identifier); pending != pending_callbacks_.end()) {
        auto& target = callbacks_[handle];
        auto& pending_list = pending->second;
        target.insert(target.end(),
                      std::make_move_iterator(pending_list.begin()),
                      std::make_move_iterator(pending_list.end()));
        pending_callbacks_.erase(pending);
    }

    const auto current_write = detail::checked_last_write_time(descriptor.source, "texture");
    const bool needs_reload = inserted || asset->last_write != current_write;
    if (needs_reload) {
        reload_asset(handle, *asset, !inserted);
    }

    return *asset;
}

bool TextureCache::contains(const TextureHandle& handle) const
{
    return handle.is_valid(assets_);
}

const TextureAsset& TextureCache::get(const TextureHandle& handle) const
{
    if (!handle.is_valid(assets_)) {
        throw std::out_of_range("Texture asset handle not found");
    }
    return assets_.get(handle.raw_handle());
}

void TextureCache::unload(const TextureHandle& handle)
{
    if (!handle.is_bound()) {
        return;
    }

    const auto raw = handle.raw_handle();
    if (!assets_.is_valid(raw)) {
        handle.reset_binding();
        return;
    }

    const auto identifier = assets_.get(raw).descriptor.handle.id();

    if (auto cb_it = callbacks_.find(raw); cb_it != callbacks_.end()) {
        if (!identifier.empty()) {
            auto& pending = pending_callbacks_[identifier];
            pending.insert(pending.end(),
                           std::make_move_iterator(cb_it->second.begin()),
                           std::make_move_iterator(cb_it->second.end()));
        }
        callbacks_.erase(cb_it);
    }

    assets_.release(raw);
    bindings_.erase(identifier);
    handle.reset_binding();
}

void TextureCache::register_hot_reload_callback(const TextureHandle& handle, HotReloadCallback callback)
{
    if (handle.is_bound() && handle.is_valid(assets_)) {
        callbacks_[handle.raw_handle()].push_back(std::move(callback));
        return;
    }

    if (handle.id().empty()) {
        throw std::invalid_argument("Texture handle identifier cannot be empty");
    }

    pending_callbacks_[handle.id()].push_back(std::move(callback));
}

void TextureCache::poll()
{
    assets_.for_each([&](const RawHandle& handle, TextureAsset& asset) {
        const auto current_write = detail::checked_last_write_time(asset.descriptor.source, "texture");
        if (current_write != asset.last_write) {
            reload_asset(handle, asset, true);
        }
    });
}

void TextureCache::reload_asset(const RawHandle& handle, TextureAsset& asset, bool notify)
{
    read_binary(asset.descriptor.source, asset.data);
    asset.last_write = detail::checked_last_write_time(asset.descriptor.source, "texture");

    if (notify) {
        const auto cb_it = callbacks_.find(handle);
        if (cb_it != callbacks_.end()) {
            for (const auto& callback : cb_it->second) {
                callback(asset);
            }
        }
    }
}

}  // namespace engine::assets

