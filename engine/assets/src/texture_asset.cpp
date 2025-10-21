#include "engine/assets/texture_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"
#include "engine/assets/detail/reload_utils.hpp"
#include "engine/assets/validation.hpp"

#include <cassert>
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

TextureCache::TextureCache()
    : handle_validator_registration_(HandleValidatorRegistry::instance().register_texture_validator(
          [this](const TextureHandle& handle) {
              std::scoped_lock lock{mutex_};
              return handle.is_valid(assets_);
          }))
{
}

const TextureAsset& TextureCache::load(const TextureAssetDescriptor& descriptor)
{
    std::scoped_lock lock{mutex_};

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
        if (auto reload = reload_asset(handle, *asset, !inserted); !reload.has_value()) {
            const auto message = reload.error().message();
            throw std::runtime_error(message.empty() ? std::string{to_string(reload.error().code())}
                                                     : std::string{message});
        }
    }

    register_watch_locked(handle, *asset);

    return *asset;
}

bool TextureCache::contains(const TextureHandle& handle) const
{
    std::scoped_lock lock{mutex_};
    return handle.is_valid(assets_);
}

const TextureAsset& TextureCache::get(const TextureHandle& handle) const
{
    std::scoped_lock lock{mutex_};
    if (!handle.is_valid(assets_))
    {
        HandleValidationTelemetry::instance().record_failure(
            HandleValidationFailure{std::string{"TextureHandle"}, handle.id(), "TextureCache::get", "Cache lookup rejected handle"});
#ifndef NDEBUG
        assert(false && "Texture asset handle not found");
#endif
        throw std::out_of_range("Texture asset handle not found");
    }
    HandleValidationTelemetry::instance().record_success("TextureHandle", handle.id());
    return assets_.get(handle.raw_handle());
}

void TextureCache::unload(const TextureHandle& handle)
{
    std::scoped_lock lock{mutex_};
    if (!handle.is_bound()) {
        return;
    }

    const auto raw = handle.raw_handle();
    if (!assets_.is_valid(raw)) {
        handle.reset_binding();
        return;
    }

    const auto identifier = assets_.get(raw).descriptor.handle.id();

    unregister_watch_locked(raw);

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
    std::scoped_lock lock{mutex_};
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
    watcher_.poll();

    std::scoped_lock lock{mutex_};
    assets_.for_each([&](const RawHandle& handle, TextureAsset& asset) {
        if (asset.descriptor.source.empty())
        {
            return;
        }

        if (watch_handles_.find(handle) != watch_handles_.end())
        {
            return;
        }

        const auto current_write = detail::checked_last_write_time(asset.descriptor.source, "texture");
        if (current_write != asset.last_write)
        {
            if (auto reload = reload_asset(handle, asset, true); !reload.has_value())
            {
                return;
            }
            register_watch_locked(handle, asset);
        }
    });
}

engine::Result<void, AssetLoadError> TextureCache::reload_asset(const RawHandle& handle,
                                                               TextureAsset& asset,
                                                               bool notify)
{
    const std::string identifier = asset.descriptor.handle.id();
    detail::record_hot_reload_attempt(notify, identifier);

    std::vector<std::byte> loaded_data{};
    try
    {
        read_binary(asset.descriptor.source, loaded_data);
    }
    catch (const std::exception& ex)
    {
        auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
        detail::record_hot_reload_failure(
            notify, identifier, error,
            "Verify the texture path exists and is readable by the runtime.");
        return error;
    }

    std::filesystem::file_time_type last_write{};
    try
    {
        last_write = detail::checked_last_write_time(asset.descriptor.source, "texture");
    }
    catch (const std::runtime_error& ex)
    {
        auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
        detail::record_hot_reload_failure(
            notify, identifier, error,
            "Ensure the texture file remains on disk and the watcher has permission to read it.");
        return error;
    }

    asset.data = std::move(loaded_data);
    asset.last_write = last_write;

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

    return {};
}

void TextureCache::register_watch_locked(const RawHandle& handle, TextureAsset& asset)
{
    if (asset.descriptor.source.empty())
    {
        unregister_watch_locked(handle);
        return;
    }

    if (auto existing = watch_handles_.find(handle); existing != watch_handles_.end())
    {
        watcher_.unwatch(existing->second);
        watch_handles_.erase(existing);
    }

    auto callback = [this, handle](const platform::filesystem::WatchEvent& event) {
        if (event.type == platform::filesystem::WatchEventType::erased)
        {
            std::scoped_lock lock{mutex_};
            if (!assets_.is_valid(handle))
            {
                return;
            }

            assets_.get(handle).last_write = event.timestamp;
            return;
        }

        std::scoped_lock lock{mutex_};
        if (!assets_.is_valid(handle))
        {
            return;
        }

        auto& tracked = assets_.get(handle);
        if (auto reload = reload_asset(handle, tracked, true); !reload.has_value())
        {
            return;
        }
    };

    const auto watch_handle = watcher_.watch_file(asset.descriptor.source, std::move(callback));
    watch_handles_.emplace(handle, watch_handle);
}

void TextureCache::unregister_watch_locked(const RawHandle& handle)
{
    if (auto it = watch_handles_.find(handle); it != watch_handles_.end())
    {
        watcher_.unwatch(it->second);
        watch_handles_.erase(it);
    }
}

}  // namespace engine::assets

