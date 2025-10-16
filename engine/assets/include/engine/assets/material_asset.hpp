#pragma once

#include "engine/assets/handles.hpp"
#include "engine/assets/shader_asset.hpp"
#include "engine/assets/texture_asset.hpp"

#include "engine/core/memory/resource_pool.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace engine::assets {

struct MaterialAssetDescriptor {
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
        return MaterialAssetDescriptor{handle,
                                       std::move(name),
                                       std::move(vertex),
                                       std::move(fragment),
                                       std::move(textures)};
    }
};

struct MaterialAsset {
    MaterialAssetDescriptor descriptor{};
};

class MaterialCache {
public:
    [[nodiscard]] const MaterialAsset& load(const MaterialAssetDescriptor& descriptor);
    [[nodiscard]] bool contains(const MaterialHandle& handle) const;
    [[nodiscard]] const MaterialAsset& get(const MaterialHandle& handle) const;

    void unload(const MaterialHandle& handle);

private:
    using Pool = core::memory::ResourcePool<MaterialAsset, MaterialHandleTag>;
    using RawHandle = typename Pool::handle_type;

    Pool assets_{};
    std::unordered_map<std::string, RawHandle> bindings_{};
};

}  // namespace engine::assets

