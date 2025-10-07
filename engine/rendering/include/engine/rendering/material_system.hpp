#pragma once

#include <optional>
#include <unordered_map>

#include "engine/assets/handles.hpp"
#include "engine/rendering/render_pass.hpp"

namespace engine::rendering
{
    /**
     * \brief Records material metadata and orchestrates GPU residency.
     */
    class MaterialSystem
    {
    public:
        struct MaterialRecord
        {
            assets::MaterialHandle material;
            assets::ShaderHandle shader;
        };

        void register_material(MaterialRecord record);

        [[nodiscard]] bool has_material(const assets::MaterialHandle& handle) const noexcept;

        [[nodiscard]] std::optional<MaterialRecord> find(const assets::MaterialHandle& handle) const;

        void ensure_material_loaded(const assets::MaterialHandle& handle, RenderResourceProvider& provider);

        void clear() noexcept;

    private:
        std::unordered_map<assets::MaterialHandle, MaterialRecord> materials_{};
    };
}
