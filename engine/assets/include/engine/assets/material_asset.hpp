#pragma once

#include "engine/assets/detail/cache_common.hpp"
#include "engine/assets/handles.hpp"
#include "engine/assets/shader_asset.hpp"
#include "engine/assets/texture_asset.hpp"

#include "engine/core/memory/resource_pool.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine::assets
{
    struct MaterialAssetDescriptor
    {
        MaterialHandle handle;
        std::string name;
        ShaderHandle vertex_shader;
        ShaderHandle fragment_shader;
        std::vector<TextureHandle> textures;

        [[nodiscard]] static MaterialAssetDescriptor from_handles(const MaterialHandle& handle,
                                                                  std::string name,
                                                                  ShaderHandle vertex,
                                                                  ShaderHandle fragment,
                                                                  std::vector<TextureHandle> textures = {})
        {
            return MaterialAssetDescriptor{
                handle,
                std::move(name),
                std::move(vertex),
                std::move(fragment),
                std::move(textures)
            };
        }
    };

    struct MaterialAsset
    {
        MaterialAssetDescriptor descriptor{};
    };

    class MaterialCache : public detail::AssetCacheLifecycle<
                              MaterialCache,
                              MaterialAsset,
                              MaterialAssetDescriptor,
                              MaterialHandle,
                              MaterialHandleTag,
                              std::function<void(const MaterialAsset&)>,
                              false>
    {
    public:
        using Base = detail::AssetCacheLifecycle<
            MaterialCache,
            MaterialAsset,
            MaterialAssetDescriptor,
            MaterialHandle,
            MaterialHandleTag,
            std::function<void(const MaterialAsset&)>,
            false>;

        MaterialCache();

        using Base::for_each_asset;

        [[nodiscard]] const MaterialAsset& load(const MaterialAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const MaterialHandle& handle) const;
        [[nodiscard]] const MaterialAsset& get(const MaterialHandle& handle) const;

        void unload(const MaterialHandle& handle);

    private:
        using typename Base::RawHandle;

        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets