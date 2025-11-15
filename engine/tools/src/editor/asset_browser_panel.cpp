#include "engine/tools/editor/asset_browser_panel.hpp"

#include "engine/tools/profiling/profiler.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#if ENGINE_ENABLE_ASSETS
#    include "engine/io/geometry_io.hpp"
#endif

namespace engine::tools::editor
{
    namespace
    {
        [[nodiscard]] std::string to_lower_copy(std::string_view value)
        {
            std::string lowered;
            lowered.reserve(value.size());
            std::transform(value.begin(), value.end(), std::back_inserter(lowered), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return lowered;
        }

        template <typename Value>
        void append_metadata_entry(
            std::vector<AssetBrowserPanel::MetadataEntry>& metadata,
            std::string key,
            Value&& value)
        {
            std::string converted = std::forward<Value>(value);
            if (!converted.empty())
            {
                metadata.emplace_back(AssetBrowserPanel::MetadataEntry{std::move(key), std::move(converted)});
            }
        }

#if ENGINE_ENABLE_ASSETS
        [[nodiscard]] std::string detection_label(const assets::AssetType type,
                                                  const io::GeometryDetectionResult& detection)
        {
            std::ostringstream stream;
            switch (type)
            {
            case assets::AssetType::mesh:
                stream << detection.mesh_format;
                break;
            case assets::AssetType::graph:
                stream << detection.graph_format;
                break;
            case assets::AssetType::point_cloud:
                stream << detection.point_cloud_format;
                break;
            default:
                break;
            }
            return stream.str();
        }

        [[nodiscard]] constexpr std::string_view shader_stage_label(assets::ShaderStage stage) noexcept
        {
            switch (stage)
            {
            case assets::ShaderStage::vertex:
                return "vertex";
            case assets::ShaderStage::fragment:
                return "fragment";
            case assets::ShaderStage::compute:
                return "compute";
            }
            return "unknown";
        }

        [[nodiscard]] constexpr std::string_view texture_color_space_label(assets::TextureColorSpace space) noexcept
        {
            switch (space)
            {
            case assets::TextureColorSpace::linear:
                return "linear";
            case assets::TextureColorSpace::srgb:
                return "srgb";
            }
            return "unknown";
        }

        [[nodiscard]] constexpr std::string_view texture_format_label(assets::TextureFormat format) noexcept
        {
            switch (format)
            {
            case assets::TextureFormat::unknown:
                return "unknown";
            case assets::TextureFormat::rgba8_unorm:
                return "rgba8_unorm";
            case assets::TextureFormat::rgba32_float:
                return "rgba32_float";
            }
            return "invalid";
        }

        template <typename Handle>
        [[nodiscard]] std::string handle_identifier(const Handle& handle)
        {
            return handle.id();
        }

        template <typename Container>
        [[nodiscard]] std::size_t total_bytes(const Container& container)
        {
            std::size_t total{0};
            for (const auto& level : container)
            {
                total += level.texels.size();
            }
            return total;
        }
#endif
    } // namespace

    AssetBrowserPanel::AssetBrowserPanel() = default;

    void AssetBrowserPanel::set_rows(std::vector<AssetDescriptorRow> rows)
    {
        std::sort(rows.begin(), rows.end(), [](const AssetDescriptorRow& lhs, const AssetDescriptorRow& rhs) {
            if (lhs.type != rhs.type)
            {
                return static_cast<int>(lhs.type) < static_cast<int>(rhs.type);
            }
            return lhs.identifier < rhs.identifier;
        });

        rows_ = std::move(rows);
        ensure_selection_valid();
    }

    void AssetBrowserPanel::set_filter_text(std::string filter)
    {
        filter_text_ = std::move(filter);
        filter_text_lower_ = to_lower_copy(filter_text_);
    }

    const std::vector<AssetBrowserPanel::AssetDescriptorRow>& AssetBrowserPanel::rows() const noexcept
    {
        return rows_;
    }

    const std::vector<const AssetBrowserPanel::AssetDescriptorRow*>& AssetBrowserPanel::filtered_rows() const
    {
        filtered_cache_.clear();
        filtered_cache_.reserve(rows_.size());
        for (const auto& row : rows_)
        {
            if (matches_filter(row))
            {
                filtered_cache_.push_back(&row);
            }
        }
        return filtered_cache_;
    }

    void AssetBrowserPanel::set_selection_key(std::string key)
    {
        selected_key_ = std::move(key);
        ensure_selection_valid();
    }

    const AssetBrowserPanel::AssetDescriptorRow* AssetBrowserPanel::selected_row() const noexcept
    {
        if (selected_key_.empty())
        {
            return nullptr;
        }

        for (const auto& row : rows_)
        {
            if (make_row_key(row) == selected_key_)
            {
                return &row;
            }
        }
        return nullptr;
    }

    void AssetBrowserPanel::render(const imgui::PanelRenderContext&)
    {
        PROFILE_SCOPE("AssetBrowserPanel");

        if (ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }

        if (!ImGui::Begin("Asset Browser"))
        {
            ImGui::End();
            return;
        }

        const auto& filtered = filtered_rows();
        render_toolbar(filtered);
        ImGui::Separator();
        render_asset_table(filtered);
        render_details();

        ImGui::End();
    }

    void AssetBrowserPanel::render_toolbar(const std::vector<const AssetDescriptorRow*>& filtered)
    {
        ImGui::PushID("AssetBrowserToolbar");

        if (ImGui::InputTextWithHint("##AssetFilter", "Filter assets...", &filter_text_))
        {
            filter_text_lower_ = to_lower_copy(filter_text_);
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear") && !filter_text_.empty())
        {
            set_filter_text({});
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%zu / %zu assets", filtered.size(), rows_.size());

        ImGui::PopID();
    }

    void AssetBrowserPanel::render_asset_table(const std::vector<const AssetDescriptorRow*>& filtered)
    {
        if (filtered.empty())
        {
            ImGui::TextDisabled("No assets match the current filter.");
            return;
        }

        constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable;

        if (ImGui::BeginTable("AssetBrowser.Table", 5, table_flags, ImVec2(0.0F, 240.0F)))
        {
            ImGui::TableSetupColumn("Identifier", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Source");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Hot Reload");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(filtered.size()));
            while (clipper.Step())
            {
                for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
                {
                    const auto* row = filtered[static_cast<std::size_t>(index)];
                    const std::string key = make_row_key(*row);

                    ImGui::TableNextRow();
                    ImGui::PushID(index);

                    ImGui::TableSetColumnIndex(0);
                    const bool is_selected = (selected_key_ == key);
                    const ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns;
                    if (ImGui::Selectable(row->identifier.c_str(), is_selected, selectable_flags))
                    {
                        selected_key_ = key;
                    }

                    ImGui::TableSetColumnIndex(1);
                    const std::string type_label{assets::to_string(row->type)};
                    ImGui::TextUnformatted(type_label.c_str());

                    ImGui::TableSetColumnIndex(2);
                    if (!row->source_path.empty())
                    {
                        const std::string path_str = row->source_path.generic_string();
                        ImGui::TextUnformatted(path_str.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled("n/a");
                    }

                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(row->status.c_str());

                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(row->hot_reload_enabled ? "enabled" : "disabled");

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
    }

    void AssetBrowserPanel::render_details() const
    {
        const auto* row = selected_row();
        if (row == nullptr)
        {
            ImGui::Separator();
            ImGui::TextDisabled("Select an asset to inspect metadata.");
            return;
        }

        ImGui::Separator();
        const std::string type_label{assets::to_string(row->type)};
        ImGui::Text("Identifier: %s", row->identifier.c_str());
        ImGui::Text("Type: %s", type_label.c_str());
        if (!row->source_path.empty())
        {
            ImGui::Text("Source: %s", row->source_path.generic_string().c_str());
        }
        else
        {
            ImGui::Text("Source: %s", "n/a");
        }
        ImGui::Text("Status: %s", row->status.c_str());
        ImGui::Text("Hot reload: %s", row->hot_reload_enabled ? "enabled" : "disabled");

        if (ImGui::BeginTable("AssetBrowser.Metadata", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 140.0F);
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (const auto& entry : row->metadata)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(entry.key.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(entry.value.c_str());
            }
            ImGui::EndTable();
        }
    }

    std::string AssetBrowserPanel::make_row_key(const AssetDescriptorRow& row) const
    {
        std::string key;
        const auto type_value = static_cast<int>(row.type);
        key.append(std::to_string(type_value));
        key.push_back(':');
        key.append(row.identifier);
        return key;
    }

    bool AssetBrowserPanel::matches_filter(const AssetDescriptorRow& row) const
    {
        if (filter_text_lower_.empty())
        {
            return true;
        }

        const auto query = filter_text_lower_;
        const auto matches = [&](std::string_view text) {
            if (text.empty())
            {
                return false;
            }
            const std::string lowered = to_lower_copy(text);
            return lowered.find(query) != std::string::npos;
        };

        if (matches(row.identifier))
        {
            return true;
        }

        if (matches(assets::to_string(row.type)))
        {
            return true;
        }

        if (!row.source_path.empty() && matches(row.source_path.generic_string()))
        {
            return true;
        }

        if (matches(row.status))
        {
            return true;
        }

        if (matches(row.hot_reload_enabled ? "hot reload enabled" : "hot reload disabled"))
        {
            return true;
        }

        if (row.gpu_resident && matches("gpu resident"))
        {
            return true;
        }

        for (const auto& entry : row.metadata)
        {
            if (matches(entry.key) || matches(entry.value))
            {
                return true;
            }
        }

        return false;
    }

    void AssetBrowserPanel::ensure_selection_valid()
    {
        if (selected_key_.empty())
        {
            return;
        }

        for (const auto& row : rows_)
        {
            if (make_row_key(row) == selected_key_)
            {
                return;
            }
        }

        selected_key_.clear();
    }

#if ENGINE_ENABLE_ASSETS
    std::vector<AssetBrowserPanel::AssetDescriptorRow> collect_asset_rows(const AssetRegistryFacade& facade)
    {
        std::vector<AssetBrowserPanel::AssetDescriptorRow> rows;
        rows.reserve(32);

        const auto make_status = [](const std::filesystem::path& path) {
            return path.empty() ? std::string{"generated"} : std::string{"loaded"};
        };

        if (facade.mesh_cache != nullptr)
        {
            facade.mesh_cache->for_each_asset([&](const auto&, const assets::MeshAsset& asset) {
                AssetBrowserPanel::AssetDescriptorRow row{};
                row.identifier = asset.descriptor.handle.id();
                row.type = assets::AssetType::mesh;
                row.source_path = asset.descriptor.source;
                row.status = make_status(asset.descriptor.source);
                row.hot_reload_enabled = true;

                append_metadata_entry(row.metadata, "format", detection_label(row.type, asset.detection));
                append_metadata_entry(row.metadata, "hint", asset.detection.format_hint);
                append_metadata_entry(row.metadata, "vertices",
                    std::to_string(asset.mesh.interface.vertex_count()));
                append_metadata_entry(row.metadata, "faces",
                    std::to_string(asset.mesh.interface.face_count()));

                rows.emplace_back(std::move(row));
            });
        }

        if (facade.graph_cache != nullptr)
        {
            facade.graph_cache->for_each_asset([&](const auto&, const assets::GraphAsset& asset) {
                AssetBrowserPanel::AssetDescriptorRow row{};
                row.identifier = asset.descriptor.handle.id();
                row.type = assets::AssetType::graph;
                row.source_path = asset.descriptor.source;
                row.status = make_status(asset.descriptor.source);
                row.hot_reload_enabled = true;

                append_metadata_entry(row.metadata, "format", detection_label(row.type, asset.detection));
                append_metadata_entry(row.metadata, "hint", asset.detection.format_hint);
                append_metadata_entry(row.metadata, "vertices",
                    std::to_string(asset.graph.interface.vertex_count()));
                append_metadata_entry(row.metadata, "edges",
                    std::to_string(asset.graph.interface.edge_count()));

                rows.emplace_back(std::move(row));
            });
        }

        if (facade.point_cloud_cache != nullptr)
        {
            facade.point_cloud_cache->for_each_asset([&](const auto&, const assets::PointCloudAsset& asset) {
                AssetBrowserPanel::AssetDescriptorRow row{};
                row.identifier = asset.descriptor.handle.id();
                row.type = assets::AssetType::point_cloud;
                row.source_path = asset.descriptor.source;
                row.status = make_status(asset.descriptor.source);
                row.hot_reload_enabled = true;

                append_metadata_entry(row.metadata, "format", detection_label(row.type, asset.detection));
                append_metadata_entry(row.metadata, "hint", asset.detection.format_hint);
                append_metadata_entry(row.metadata, "points",
                    std::to_string(asset.point_cloud.interface.vertex_count()));

                rows.emplace_back(std::move(row));
            });
        }

        if (facade.texture_cache != nullptr)
        {
            facade.texture_cache->for_each_asset([&](const auto&, const assets::TextureAsset& asset) {
                AssetBrowserPanel::AssetDescriptorRow row{};
                row.identifier = asset.descriptor.handle.id();
                row.type = assets::AssetType::texture;
                row.source_path = asset.descriptor.source;
                row.status = make_status(asset.descriptor.source);
                row.hot_reload_enabled = true;

                append_metadata_entry(row.metadata, "dimensions",
                    std::to_string(asset.dimensions.width) + "x" +
                        std::to_string(asset.dimensions.height));
                append_metadata_entry(row.metadata, "depth",
                    std::to_string(asset.dimensions.depth));
                append_metadata_entry(row.metadata, "mip_levels",
                    std::to_string(asset.mip_levels.size()));
                append_metadata_entry(row.metadata, "color_space",
                    std::string(texture_color_space_label(asset.descriptor.color_space)));
                append_metadata_entry(row.metadata, "format",
                    std::string(texture_format_label(asset.format)));
                append_metadata_entry(row.metadata, "encoded_bytes",
                    std::to_string(asset.encoded_payload.size()));
                append_metadata_entry(row.metadata, "texel_bytes",
                    std::to_string(total_bytes(asset.mip_levels)));

                rows.emplace_back(std::move(row));
            });
        }

        if (facade.shader_cache != nullptr)
        {
            facade.shader_cache->for_each_asset([&](const auto&, const assets::ShaderAsset& asset) {
                AssetBrowserPanel::AssetDescriptorRow row{};
                row.identifier = asset.descriptor.handle.id();
                row.type = assets::AssetType::shader;
                row.source_path = asset.descriptor.source;
                row.status = make_status(asset.descriptor.source);
                row.hot_reload_enabled = true;

                append_metadata_entry(row.metadata, "stage",
                    std::string(shader_stage_label(asset.descriptor.stage)));
                append_metadata_entry(row.metadata, "spirv_words",
                    std::to_string(asset.binary.spirv.size()));
                append_metadata_entry(row.metadata, "optimised",
                    asset.descriptor.options.optimize ? "true" : "false");

                rows.emplace_back(std::move(row));
            });
        }

        if (facade.material_cache != nullptr)
        {
            facade.material_cache->for_each_asset([&](const auto&, const assets::MaterialAsset& asset) {
                AssetBrowserPanel::AssetDescriptorRow row{};
                row.identifier = asset.descriptor.handle.id();
                row.type = assets::AssetType::material;
                row.status = "loaded";
                row.hot_reload_enabled = false;

                append_metadata_entry(row.metadata, "name", asset.descriptor.name);
                append_metadata_entry(row.metadata, "vertex_shader",
                    handle_identifier(asset.descriptor.vertex_shader));
                append_metadata_entry(row.metadata, "fragment_shader",
                    handle_identifier(asset.descriptor.fragment_shader));
                append_metadata_entry(row.metadata, "texture_count",
                    std::to_string(asset.descriptor.textures.size()));

                rows.emplace_back(std::move(row));
            });
        }

        return rows;
    }
#endif
} // namespace engine::tools::editor

