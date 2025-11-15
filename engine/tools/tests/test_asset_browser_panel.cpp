#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include "engine/assets/material_asset.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/tools/editor/asset_browser_panel.hpp"

namespace
{
    using engine::assets::AssetType;
    using engine::assets::MaterialAssetDescriptor;
    using engine::assets::MaterialCache;
    using engine::assets::MaterialHandle;
    using engine::assets::MeshAssetDescriptor;
    using engine::assets::MeshCache;
    using engine::tools::editor::AssetBrowserPanel;
    using engine::tools::editor::AssetRegistryFacade;
    using engine::tools::editor::collect_asset_rows;
}

TEST(AssetBrowserPanel, SortsRowsByTypeAndIdentifier)
{
    AssetBrowserPanel panel;
    std::vector<AssetBrowserPanel::AssetDescriptorRow> rows{};
    rows.push_back(AssetBrowserPanel::AssetDescriptorRow{
        .identifier = "b_mesh",
        .type = AssetType::mesh,
        .status = "loaded",
    });
    rows.push_back(AssetBrowserPanel::AssetDescriptorRow{
        .identifier = "a_material",
        .type = AssetType::material,
        .status = "loaded",
    });
    rows.push_back(AssetBrowserPanel::AssetDescriptorRow{
        .identifier = "a_mesh",
        .type = AssetType::mesh,
        .status = "loaded",
    });

    panel.set_rows(std::move(rows));

    const auto& ordered = panel.rows();
    ASSERT_EQ(ordered.size(), 3U);
    EXPECT_EQ(ordered[0].type, AssetType::mesh);
    EXPECT_EQ(ordered[0].identifier, "a_mesh");
    EXPECT_EQ(ordered[1].identifier, "b_mesh");
    EXPECT_EQ(ordered[2].identifier, "a_material");
}

TEST(AssetBrowserPanel, FiltersRowsByMetadata)
{
    AssetBrowserPanel panel;
    AssetBrowserPanel::AssetDescriptorRow mesh_row{};
    mesh_row.identifier = "mesh";
    mesh_row.type = AssetType::mesh;
    mesh_row.metadata.push_back({"vertices", "24"});

    AssetBrowserPanel::AssetDescriptorRow material_row{};
    material_row.identifier = "material";
    material_row.type = AssetType::material;

    panel.set_rows({mesh_row, material_row});
    panel.set_filter_text("24");

    const auto& filtered = panel.filtered_rows();
    ASSERT_EQ(filtered.size(), 1U);
    EXPECT_EQ(filtered.front()->identifier, "mesh");
}

TEST(AssetBrowserPanel, CollectAssetRowsAggregatesCaches)
{
    const auto mesh_path = std::filesystem::temp_directory_path()
        / std::filesystem::path("asset-browser-"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".obj");

    {
        std::ofstream stream(mesh_path);
        ASSERT_TRUE(stream.is_open());
        stream << "v 0 0 0\n";
        stream << "v 1 0 0\n";
        stream << "v 0 1 0\n";
        stream << "f 1 2 3\n";
    }

    MeshCache mesh_cache;
    const auto mesh_descriptor = MeshAssetDescriptor::from_file(mesh_path);
    const auto& mesh_asset = mesh_cache.load(mesh_descriptor);

    MaterialCache material_cache;
    MaterialAssetDescriptor material_descriptor = MaterialAssetDescriptor::from_handles(
        MaterialHandle{std::string{"material://test"}},
        "TestMaterial",
        engine::assets::ShaderHandle{std::string{"shader://vertex"}},
        engine::assets::ShaderHandle{std::string{"shader://fragment"}}
    );
    [[maybe_unused]] const auto& material_asset = material_cache.load(material_descriptor);

    AssetRegistryFacade facade{};
    facade.mesh_cache = &mesh_cache;
    facade.material_cache = &material_cache;

    const auto rows = collect_asset_rows(facade);
    EXPECT_GE(rows.size(), 2U);

    const auto find_row = [&](AssetType type) -> const AssetBrowserPanel::AssetDescriptorRow*
    {
        for (const auto& row : rows)
        {
            if (row.type == type)
            {
                return &row;
            }
        }
        return nullptr;
    };

    const auto* mesh_row = find_row(AssetType::mesh);
    ASSERT_NE(mesh_row, nullptr);
    EXPECT_EQ(mesh_row->status, "loaded");
    bool has_vertex_entry = false;
    for (const auto& entry : mesh_row->metadata)
    {
        if (entry.key == "vertices")
        {
            has_vertex_entry = true;
            break;
        }
    }
    EXPECT_TRUE(has_vertex_entry);

    const auto* material_row = find_row(AssetType::material);
    ASSERT_NE(material_row, nullptr);
    EXPECT_FALSE(material_row->hot_reload_enabled);

    const auto mesh_handle = mesh_asset.descriptor.handle;
    mesh_cache.unload(mesh_handle);
    std::filesystem::remove(mesh_path);
}

