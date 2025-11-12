#pragma once

#include <functional>
#include <utility>

#include <functional>
#include <memory>

#include <entt/entt.hpp>

#include "engine/tools/editor/scene_hierarchy_panel.hpp"
#include "engine/tools/imgui/panel_registry.hpp"

namespace engine::runtime
{
    struct RuntimeDiagnostics;
}

namespace engine::scene::validation
{
    struct HierarchyValidationReport;
}

namespace engine::tools::editor
{
    /// Bridge that registers runtime-driven Dear ImGui panels with the shared panel registry.
    class RuntimePanelBridge
    {
    public:
        using DiagnosticsProvider = std::function<const runtime::RuntimeDiagnostics&()>;
        using SceneValidationProvider =
            std::function<const scene::validation::HierarchyValidationReport*()>;
        using DiagnosticsRenderer = std::function<void(const runtime::RuntimeDiagnostics&)>;
        using ProfilerRenderer = std::function<void(bool*)>;
        using SceneValidationRenderer =
            std::function<void(const scene::validation::HierarchyValidationReport&)>;
        using SceneProvider = std::function<scene::Scene*()>;
        using HierarchySelectionProvider = std::function<entt::entity()>;
        using HierarchySelectionCallback = std::function<void(entt::entity)>;

        struct Renderers
        {
            DiagnosticsRenderer diagnostics;
            ProfilerRenderer profiler;
            SceneValidationRenderer scene_validation;
        };

        struct HierarchyPanelHooks
        {
            SceneProvider scene_provider;
            HierarchySelectionProvider selection_provider;
            HierarchySelectionCallback selection_callback;
        };

        RuntimePanelBridge(
            imgui::PanelRegistry& registry,
            DiagnosticsProvider diagnostics_provider,
            SceneValidationProvider scene_validation_provider = SceneValidationProvider{},
            Renderers renderers = Renderers{},
            HierarchyPanelHooks hierarchy_hooks = HierarchyPanelHooks{}
        );

        /// Render all registered panels using \p delta_time for the render context.
        void render_all(double delta_time) const;

        /// Control whether the profiler window should be visible on the next render.
        void set_profiler_visible(bool visible) noexcept;

        /// Return true when the profiler window is currently marked visible.
        [[nodiscard]] bool profiler_visible() const noexcept;

    private:
        imgui::PanelRegistry* registry_{};
        DiagnosticsProvider diagnostics_provider_{};
        SceneValidationProvider scene_validation_provider_{};
        Renderers renderers_{};
        mutable bool profiler_visible_{true};
        imgui::PanelRegistry::RegistrationHandle diagnostics_handle_{};
        imgui::PanelRegistry::RegistrationHandle profiler_handle_{};
        imgui::PanelRegistry::RegistrationHandle scene_validation_handle_{};
        HierarchyPanelHooks hierarchy_hooks_{};
        std::unique_ptr<SceneHierarchyPanel> hierarchy_panel_{};
        imgui::PanelRegistry::RegistrationHandle hierarchy_handle_{};
    };
} // namespace engine::tools::editor

