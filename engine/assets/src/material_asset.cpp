#include "engine/assets/material_asset.hpp"

#include <stdexcept>

namespace engine::assets {

const MaterialAsset& MaterialCache::load(const MaterialAssetDescriptor& descriptor)
{
    const auto identifier = descriptor.handle.id();
    if (identifier.empty()) {
        throw std::invalid_argument("Material handle identifier cannot be empty");
    }

    MaterialAsset* asset = nullptr;
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

    (void)inserted;

    asset->descriptor = descriptor;
    descriptor.handle.bind(handle);

    return *asset;
}

bool MaterialCache::contains(const MaterialHandle& handle) const
{
    return handle.is_valid(assets_);
}

const MaterialAsset& MaterialCache::get(const MaterialHandle& handle) const
{
    if (!handle.is_valid(assets_)) {
        throw std::out_of_range("Material asset handle not found");
    }
    return assets_.get(handle.raw_handle());
}

void MaterialCache::unload(const MaterialHandle& handle)
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
    assets_.release(raw);
    bindings_.erase(identifier);
    handle.reset_binding();
}

}  // namespace engine::assets

