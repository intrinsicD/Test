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
            engine::assets::MaterialHandle material;
            engine::assets::ShaderHandle shader;
        };

        void register_material(MaterialRecord record);

        [[nodiscard]] bool has_material(const engine::assets::MaterialHandle& handle) const noexcept;

        [[nodiscard]] std::optional<MaterialRecord> find(const engine::assets::MaterialHandle& handle) const;

        void ensure_material_loaded(const engine::assets::MaterialHandle& handle, RenderResourceProvider& provider);

        void clear() noexcept;

    private:
        std::unordered_map<engine::assets::MaterialHandle, MaterialRecord> materials_{};
    };
}
