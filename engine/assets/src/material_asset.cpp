#include "engine/assets/material_asset.hpp"

#include <stdexcept>

namespace engine::assets {

const MaterialAsset& MaterialCache::load(const MaterialAssetDescriptor& descriptor)
{
    auto [it, inserted] = assets_.try_emplace(descriptor.handle);
    MaterialAsset& asset = it->second;
    asset.descriptor = descriptor;
    (void)inserted;
    return asset;
}

bool MaterialCache::contains(const MaterialHandle& handle) const
{
    return assets_.find(handle) != assets_.end();
}

const MaterialAsset& MaterialCache::get(const MaterialHandle& handle) const
{
    const auto it = assets_.find(handle);
    if (it == assets_.end()) {
        throw std::out_of_range("Material asset handle not found");
    }
    return it->second;
}

void MaterialCache::unload(const MaterialHandle& handle)
{
    assets_.erase(handle);
}

}  // namespace engine::assets

