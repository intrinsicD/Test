#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/assets/async.hpp"

#if ENGINE_ENABLE_ASSETS
#    include "engine/assets/graph_asset.hpp"
#    include "engine/assets/material_asset.hpp"
#    include "engine/assets/mesh_asset.hpp"
#    include "engine/assets/point_cloud_asset.hpp"
#    include "engine/assets/shader_asset.hpp"
#    include "engine/assets/texture_asset.hpp"
#endif

namespace engine::tools::imgui
{
    struct PanelRenderContext;
}

namespace engine::tools::editor
{
    class AssetBrowserPanel
    {
    public:
        struct MetadataEntry
        {
            std::string key{};
            std::string value{};
        };

        struct AssetDescriptorRow
        {
            std::string identifier{};
            assets::AssetType type{assets::AssetType::unknown};
            std::filesystem::path source_path{};
            std::string status{"unloaded"};
            bool hot_reload_enabled{false};
            bool gpu_resident{false};
            std::vector<MetadataEntry> metadata{};
        };

        AssetBrowserPanel();

        void set_rows(std::vector<AssetDescriptorRow> rows);
        void set_filter_text(std::string filter);
        [[nodiscard]] const std::vector<AssetDescriptorRow>& rows() const noexcept;
        [[nodiscard]] const std::vector<const AssetDescriptorRow*>& filtered_rows() const;
        void set_selection_key(std::string key);
        [[nodiscard]] const AssetDescriptorRow* selected_row() const noexcept;

        void render(const imgui::PanelRenderContext& context);

    private:
        void render_toolbar(const std::vector<const AssetDescriptorRow*>& filtered);
        void render_asset_table(const std::vector<const AssetDescriptorRow*>& filtered);
        void render_details() const;
        [[nodiscard]] std::string make_row_key(const AssetDescriptorRow& row) const;
        [[nodiscard]] bool matches_filter(const AssetDescriptorRow& row) const;
        void ensure_selection_valid();

        std::vector<AssetDescriptorRow> rows_{};
        std::string filter_text_{};
        std::string filter_text_lower_{};
        std::string selected_key_{};
        mutable std::vector<const AssetDescriptorRow*> filtered_cache_{};
    };

#if ENGINE_ENABLE_ASSETS
    struct AssetRegistryFacade
    {
        assets::MeshCache* mesh_cache{nullptr};
        assets::GraphCache* graph_cache{nullptr};
        assets::PointCloudCache* point_cloud_cache{nullptr};
        assets::TextureCache* texture_cache{nullptr};
        assets::ShaderCache* shader_cache{nullptr};
        assets::MaterialCache* material_cache{nullptr};
    };

    [[nodiscard]] std::vector<AssetBrowserPanel::AssetDescriptorRow> collect_asset_rows(
        const AssetRegistryFacade& facade);
#else
    struct AssetRegistryFacade
    {
    };

    inline std::vector<AssetBrowserPanel::AssetDescriptorRow> collect_asset_rows(const AssetRegistryFacade&)
    {
        return {};
    }
#endif
}

