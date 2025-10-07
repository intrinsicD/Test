#include "engine/rendering/material_system.hpp"

#include <utility>

namespace engine::rendering
{
    void MaterialSystem::register_material(MaterialRecord record)
    {
        materials_[record.material] = std::move(record);
    }

    bool MaterialSystem::has_material(const engine::assets::MaterialHandle& handle) const noexcept
    {
        return materials_.find(handle) != materials_.end();
    }

    std::optional<MaterialSystem::MaterialRecord> MaterialSystem::find(const engine::assets::MaterialHandle& handle) const
    {
        if (auto it = materials_.find(handle); it != materials_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    void MaterialSystem::ensure_material_loaded(const engine::assets::MaterialHandle& handle,
                                                RenderResourceProvider& provider)
    {
        if (handle.empty())
        {
            return;
        }

        provider.require_material(handle);

        if (auto record = find(handle))
        {
            if (!record->shader.empty())
            {
                provider.require_shader(record->shader);
            }
        }
    }

    void MaterialSystem::clear() noexcept
    {
        materials_.clear();
    }
}
