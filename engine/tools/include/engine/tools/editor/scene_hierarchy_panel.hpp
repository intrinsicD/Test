#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <entt/entt.hpp>

#include "engine/scene/validation.hpp"
#include "engine/tools/imgui/panel_registry.hpp"

namespace engine::scene
{
    class Scene;
}

namespace engine::tools::imgui
{
    struct PanelRenderContext;
}

namespace engine::tools::editor
{
    /// Lightweight model that exposes accessors for traversing a scene hierarchy.
    class HierarchyPanelModel
    {
    public:
        using ChildVisitor = std::function<void(entt::entity)>;

        HierarchyPanelModel() = default;
        explicit HierarchyPanelModel(scene::Scene* scene) noexcept;

        void set_scene(scene::Scene* scene) noexcept;
        [[nodiscard]] scene::Scene* scene() const noexcept;

        void synchronize_selection(entt::entity entity) noexcept;
        void set_selection(entt::entity entity) noexcept;
        [[nodiscard]] entt::entity selection() const noexcept;

        void for_each_root(const ChildVisitor& visitor) const;
        void for_each_child(entt::entity parent, const ChildVisitor& visitor) const;
        [[nodiscard]] bool valid(entt::entity entity) const noexcept;
        [[nodiscard]] entt::entity first_child(entt::entity parent) const noexcept;

    private:
        scene::Scene* scene_{nullptr};
        entt::entity selection_{entt::null};
    };

    /// Dear ImGui panel that renders the runtime scene hierarchy and validation results.
    class SceneHierarchyPanel
    {
    public:
        using SelectionCallback = std::function<void(entt::entity)>;

        SceneHierarchyPanel();
        explicit SceneHierarchyPanel(HierarchyPanelModel model);

        void set_scene(scene::Scene* scene) noexcept;
        void synchronize_external_selection(entt::entity entity) noexcept;
        void set_selection(entt::entity entity) noexcept;
        void set_selection_callback(SelectionCallback callback);

        void render(const imgui::PanelRenderContext& context);

        void refresh_validation();

        [[nodiscard]] HierarchyPanelModel& model() noexcept;
        [[nodiscard]] const HierarchyPanelModel& model() const noexcept;

    private:
        [[nodiscard]] bool has_scene() const noexcept;
        [[nodiscard]] std::string format_entity_label(entt::entity entity) const;
        void render_roots();
        void render_node(entt::entity entity);
        void render_validation_summary();
        void rebuild_issue_lookup();
        void clear_cached_state();
        void ensure_selection_visible(entt::entity entity);

        HierarchyPanelModel model_{};
        SelectionCallback selection_callback_{};
        scene::validation::HierarchyValidationReport* report_{nullptr};
        std::optional<scene::validation::HierarchyValidationReport> cached_report_{};
        std::unordered_map<entt::entity, std::size_t> entity_issue_counts_{};
        std::unordered_set<entt::entity> pending_selection_chain_{};
        bool validation_dirty_{true};
    };

    [[nodiscard]] imgui::PanelRegistry::RegistrationHandle register_scene_hierarchy_panel(
        imgui::PanelRegistry& registry,
        SceneHierarchyPanel& panel,
        std::string identifier = "editor.scene_hierarchy"
    );
}

