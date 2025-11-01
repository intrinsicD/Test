#include "engine/assets/material_asset.hpp"
#include "engine/assets/validation.hpp"

#include <cassert>
#include <stdexcept>

namespace engine::assets
{
    MaterialCache::MaterialCache()
        : Base(detail::AssetCacheLabels{
              "Material",
              "material",
              "MaterialHandle",
              "MaterialCache"})
        , handle_validator_registration_(HandleValidatorRegistry::instance().register_material_validator(
              [this](const MaterialHandle& handle)
              {
                  return handle.is_valid(this->assets_);
              }))
    {
    }

    const MaterialAsset& MaterialCache::load(const MaterialAssetDescriptor& descriptor)
    {
        std::scoped_lock lock{this->mutex_};

        auto acquisition = this->acquire_asset_slot(descriptor);
        this->bind_descriptor(descriptor, acquisition.handle, *acquisition.asset);
        return *acquisition.asset;
    }

    bool MaterialCache::contains(const MaterialHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->contains_handle(handle);
    }

    const MaterialAsset& MaterialCache::get(const MaterialHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->get_asset_checked(handle);
    }

    void MaterialCache::unload(const MaterialHandle& handle)
    {
        std::scoped_lock lock{this->mutex_};
        this->release_handle(handle);
    }
} // namespace engine::assets