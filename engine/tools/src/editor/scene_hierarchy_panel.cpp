#include "engine/tools/editor/scene_hierarchy_panel.hpp"

#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/components/name.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/validation.hpp"
#include "engine/tools/imgui_helpers.hpp"
#include "engine/tools/profiling/profiler.hpp"

#include <imgui.h>

#include <cstdint>
#include <sstream>
#include <utility>
#include <type_traits>

namespace engine::tools::editor
{
    HierarchyPanelModel::HierarchyPanelModel(scene::Scene* scene) noexcept : scene_{scene}
    {
    }

    void HierarchyPanelModel::set_scene(scene::Scene* scene) noexcept
    {
        scene_ = scene;
        if (!valid(selection_))
        {
            selection_ = entt::null;
        }
    }

    scene::Scene* HierarchyPanelModel::scene() const noexcept
    {
        return scene_;
    }

    void HierarchyPanelModel::synchronize_selection(entt::entity entity) noexcept
    {
        if (scene_ == nullptr || entity == entt::null)
        {
            selection_ = entt::null;
            return;
        }

        selection_ = valid(entity) ? entity : entt::null;
    }

    void HierarchyPanelModel::set_selection(entt::entity entity) noexcept
    {
        if (scene_ == nullptr)
        {
            selection_ = entt::null;
            return;
        }

        if (entity == entt::null)
        {
            selection_ = entt::null;
            return;
        }

        selection_ = valid(entity) ? entity : entt::null;
    }

    entt::entity HierarchyPanelModel::selection() const noexcept
    {
        return selection_;
    }

    bool HierarchyPanelModel::valid(entt::entity entity) const noexcept
    {
        return scene_ != nullptr && scene_->valid(entity);
    }

    entt::entity HierarchyPanelModel::first_child(entt::entity parent) const noexcept
    {
        if (!valid(parent))
        {
            return entt::null;
        }

        auto& registry = scene_->registry();
        if (const auto* hierarchy = registry.try_get<scene::components::Hierarchy>(parent); hierarchy != nullptr)
        {
            return hierarchy->first_child;
        }

        return entt::null;
    }

    void HierarchyPanelModel::for_each_child(entt::entity parent, const ChildVisitor& visitor) const
    {
        if (scene_ == nullptr || !visitor || !valid(parent))
        {
            return;
        }

        auto& registry = scene_->registry();
        const auto* hierarchy = registry.try_get<scene::components::Hierarchy>(parent);
        if (hierarchy == nullptr)
        {
            return;
        }

        auto child = hierarchy->first_child;
        while (child != entt::null)
        {
            if (!registry.valid(child))
            {
                break;
            }

            visitor(child);

            const auto* child_hierarchy = registry.try_get<scene::components::Hierarchy>(child);
            child = (child_hierarchy != nullptr) ? child_hierarchy->next_sibling : entt::null;
        }
    }

    void HierarchyPanelModel::for_each_root(const ChildVisitor& visitor) const
    {
        if (scene_ == nullptr || !visitor)
        {
            return;
        }

        auto& registry = scene_->registry();
        auto& entities = registry.storage<entt::entity>();
        for (auto [entity] : entities.each())
        {
            const auto* hierarchy = registry.try_get<scene::components::Hierarchy>(entity);
            const bool is_root = hierarchy == nullptr
                || hierarchy->parent == entt::null
                || !registry.valid(hierarchy->parent);
            if (is_root)
            {
                visitor(entity);
            }
        }
    }

    SceneHierarchyPanel::SceneHierarchyPanel() = default;

    SceneHierarchyPanel::SceneHierarchyPanel(HierarchyPanelModel model)
        : model_{std::move(model)}
    {
    }

    void SceneHierarchyPanel::set_scene(scene::Scene* scene) noexcept
    {
        if (model_.scene() == scene)
        {
            if (!model_.valid(model_.selection()))
            {
                model_.set_selection(entt::null);
            }
            return;
        }

        model_.set_scene(scene);
        clear_cached_state();
    }

    void SceneHierarchyPanel::synchronize_external_selection(entt::entity entity) noexcept
    {
        model_.synchronize_selection(entity);
        if (model_.selection() != entt::null)
        {
            ensure_selection_visible(model_.selection());
        }
    }

    void SceneHierarchyPanel::set_selection(entt::entity entity) noexcept
    {
        const auto previous = model_.selection();
        model_.set_selection(entity);
        const auto updated = model_.selection();
        if (updated != entt::null)
        {
            ensure_selection_visible(updated);
        }

        if (selection_callback_ && updated != previous)
        {
            selection_callback_(updated);
        }
    }

    void SceneHierarchyPanel::set_selection_callback(SelectionCallback callback)
    {
        selection_callback_ = std::move(callback);
    }

    HierarchyPanelModel& SceneHierarchyPanel::model() noexcept
    {
        return model_;
    }

    const HierarchyPanelModel& SceneHierarchyPanel::model() const noexcept
    {
        return model_;
    }

    bool SceneHierarchyPanel::has_scene() const noexcept
    {
        return model_.scene() != nullptr;
    }

    void SceneHierarchyPanel::render(const imgui::PanelRenderContext&)
    {
        PROFILE_SCOPE("SceneHierarchyPanel");

        if (ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }

        if (!ImGui::Begin("Scene Hierarchy"))
        {
            ImGui::End();
            return;
        }

        auto* scene = model_.scene();
        if (scene == nullptr)
        {
            ImGui::TextDisabled("No scene available.");
            ImGui::End();
            return;
        }

        model_.synchronize_selection(model_.selection());

        std::size_t entity_count{0};
        auto& entity_storage = scene->registry().storage<entt::entity>();
        for (auto [entity] : entity_storage.each())
        {
            (void)entity;
            ++entity_count;
        }
        ImGui::Text("Entities: %zu", entity_count);
        ImGui::SameLine();
        if (ImGui::Button("Re-run Validation"))
        {
            validation_dirty_ = true;
        }

        if (validation_dirty_)
        {
            refresh_validation();
        }

        render_validation_summary();

        ImGui::Separator();

        if (ImGui::BeginChild("SceneHierarchyTree", ImVec2(0.0F, 0.0F), false,
                              ImGuiWindowFlags_HorizontalScrollbar))
        {
            render_roots();
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void SceneHierarchyPanel::refresh_validation()
    {
        if (!has_scene())
        {
            return;
        }

        PROFILE_SCOPE("SceneHierarchyPanel.Validation");
        cached_report_ = scene::validation::validate_hierarchy(*model_.scene());
        report_ = &cached_report_.value();
        rebuild_issue_lookup();
        validation_dirty_ = false;
    }

    void SceneHierarchyPanel::render_roots()
    {
        model_.for_each_root([&](entt::entity entity) {
            render_node(entity);
        });
    }

    void SceneHierarchyPanel::render_node(entt::entity entity)
    {
        const auto* scene = model_.scene();
        if (scene == nullptr)
        {
            return;
        }

        auto& registry = scene->registry();
        const bool has_children = model_.first_child(entity) != entt::null;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!has_children)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (model_.selection() == entity)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (pending_selection_chain_.contains(entity))
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            pending_selection_chain_.erase(entity);
        }

        std::string label = format_entity_label(entity);
        const auto issue_it = entity_issue_counts_.find(entity);
        if (issue_it != entity_issue_counts_.end() && issue_it->second > 0U)
        {
            label.append("  [!]");
        }

        const bool open = ImGui::TreeNodeEx(
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(entt::to_integral(entity))),
            flags,
            "%s",
            label.c_str()
        );

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)
            && issue_it != entity_issue_counts_.end() && issue_it->second > 0U && report_ != nullptr)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Validation issues: %zu", issue_it->second);
            for (const auto& issue : report_->issues)
            {
                if (issue.entity == entity)
                {
                    ImGui::Text("- %s", issue.message.c_str());
                }
            }
            ImGui::EndTooltip();
        }

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            set_selection(entity);
        }

        if (open && has_children)
        {
            model_.for_each_child(entity, [&](entt::entity child) {
                render_node(child);
            });
            ImGui::TreePop();
        }
        else if (open)
        {
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::render_validation_summary()
    {
        if (report_ == nullptr)
        {
            ImGui::TextDisabled("Validation report unavailable.");
            return;
        }

        const auto& metrics = report_->metrics;
        ImGui::Text("Hierarchy %s (%zu issues)", report_->ok() ? "OK" : "has issues", metrics.issue_count);

        if (ImGui::CollapsingHeader("Validation Details", ImGuiTreeNodeFlags_DefaultOpen))
        {
            imgui::render_validation_report(*report_);
        }
    }

    void SceneHierarchyPanel::rebuild_issue_lookup()
    {
        entity_issue_counts_.clear();
        if (report_ == nullptr)
        {
            return;
        }

        for (const auto& issue : report_->issues)
        {
            if (issue.entity != entt::null)
            {
                ++entity_issue_counts_[issue.entity];
            }

            if (issue.related != entt::null)
            {
                ++entity_issue_counts_[issue.related];
            }
        }
    }

    void SceneHierarchyPanel::clear_cached_state()
    {
        cached_report_.reset();
        report_ = nullptr;
        entity_issue_counts_.clear();
        pending_selection_chain_.clear();
        validation_dirty_ = true;
    }

    void SceneHierarchyPanel::ensure_selection_visible(entt::entity entity)
    {
        pending_selection_chain_.clear();
        auto* scene = model_.scene();
        if (scene == nullptr || !model_.valid(entity))
        {
            return;
        }

        auto& registry = scene->registry();
        auto current = entity;
        while (current != entt::null && registry.valid(current))
        {
            pending_selection_chain_.insert(current);
            const auto* hierarchy = registry.try_get<scene::components::Hierarchy>(current);
            if (hierarchy == nullptr)
            {
                break;
            }

            current = hierarchy->parent;
        }
    }

    std::string SceneHierarchyPanel::format_entity_label(entt::entity entity) const
    {
        auto* scene = model_.scene();
        if (scene == nullptr)
        {
            return "<invalid>";
        }

        auto& registry = scene->registry();
        if (const auto* name = registry.try_get<scene::components::Name>(entity); name != nullptr)
        {
            if (!name->value.empty())
            {
                return name->value;
            }
        }

        std::ostringstream stream;
        stream << "Entity " << static_cast<std::underlying_type_t<entt::entity>>(entity);
        return stream.str();
    }

    imgui::PanelRegistry::RegistrationHandle register_scene_hierarchy_panel(
        imgui::PanelRegistry& registry,
        SceneHierarchyPanel& panel,
        std::string identifier)
    {
        return registry.register_scoped_panel(
            std::move(identifier),
            [&panel](const imgui::PanelRenderContext& context) {
                panel.render(context);
            }
        );
    }
} // namespace engine::tools::editor

