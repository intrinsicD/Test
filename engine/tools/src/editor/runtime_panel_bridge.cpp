#include "engine/tools/editor/runtime_panel_bridge.hpp"

#include "engine/tools/imgui_helpers.hpp"

#include "engine/runtime/api.hpp"
#include "engine/scene/validation.hpp"

namespace engine::tools::editor
{
    namespace
    {
        RuntimePanelBridge::DiagnosticsRenderer make_default_diagnostics_renderer()
        {
            return [](const runtime::RuntimeDiagnostics& diagnostics) {
                imgui::render_diagnostics(diagnostics);
            };
        }

        RuntimePanelBridge::ProfilerRenderer make_default_profiler_renderer()
        {
            return [](bool* visible) {
                imgui::render_profiler_window(visible);
            };
        }

        RuntimePanelBridge::SceneValidationRenderer make_default_scene_validation_renderer()
        {
            return [](const scene::validation::HierarchyValidationReport& report) {
                imgui::render_validation_report(report);
            };
        }
    } // namespace

    RuntimePanelBridge::RuntimePanelBridge(
        imgui::PanelRegistry& registry,
        DiagnosticsProvider diagnostics_provider,
        SceneValidationProvider scene_validation_provider,
        Renderers renderers,
        HierarchyPanelHooks hierarchy_hooks
    )
        : registry_(&registry)
        , diagnostics_provider_(std::move(diagnostics_provider))
        , scene_validation_provider_(std::move(scene_validation_provider))
        , renderers_(std::move(renderers))
        , hierarchy_hooks_(std::move(hierarchy_hooks))
    {
        if (!renderers_.diagnostics)
        {
            renderers_.diagnostics = make_default_diagnostics_renderer();
        }

        if (!renderers_.profiler)
        {
            renderers_.profiler = make_default_profiler_renderer();
        }

        if (!renderers_.scene_validation)
        {
            renderers_.scene_validation = make_default_scene_validation_renderer();
        }

        if (registry_ && diagnostics_provider_ && renderers_.diagnostics)
        {
            diagnostics_handle_ = registry_->register_scoped_panel(
                "runtime.diagnostics",
                [this](const imgui::PanelRenderContext&) {
                    renderers_.diagnostics(diagnostics_provider_());
                }
            );
        }

        if (registry_ && renderers_.profiler)
        {
            profiler_handle_ = registry_->register_scoped_panel(
                "runtime.profiler",
                [this](const imgui::PanelRenderContext&) {
                    renderers_.profiler(&profiler_visible_);
                }
            );
        }

        if (registry_ && scene_validation_provider_ && renderers_.scene_validation)
        {
            scene_validation_handle_ = registry_->register_scoped_panel(
                "runtime.scene_validation",
                [this](const imgui::PanelRenderContext&) {
                    if (const auto* report = scene_validation_provider_())
                    {
                        renderers_.scene_validation(*report);
                    }
                }
            );
        }

        if (registry_ && hierarchy_hooks_.scene_provider)
        {
            hierarchy_panel_ = std::make_unique<SceneHierarchyPanel>();
            if (hierarchy_hooks_.selection_callback)
            {
                hierarchy_panel_->set_selection_callback(hierarchy_hooks_.selection_callback);
            }

            hierarchy_panel_->set_scene(hierarchy_hooks_.scene_provider());
            if (hierarchy_hooks_.selection_provider)
            {
                hierarchy_panel_->synchronize_external_selection(hierarchy_hooks_.selection_provider());
            }

            hierarchy_handle_ = register_scene_hierarchy_panel(
                *registry_,
                *hierarchy_panel_,
                "editor.scene_hierarchy"
            );
        }
    }

    void RuntimePanelBridge::render_all(double delta_time) const
    {
        if (!registry_)
        {
            return;
        }

        if (hierarchy_panel_)
        {
            if (hierarchy_hooks_.scene_provider)
            {
                hierarchy_panel_->set_scene(hierarchy_hooks_.scene_provider());
            }

            if (hierarchy_hooks_.selection_provider)
            {
                hierarchy_panel_->synchronize_external_selection(hierarchy_hooks_.selection_provider());
            }
        }

        const imgui::PanelRenderContext context{.delta_time = delta_time};
        registry_->render_all(context);
    }

    void RuntimePanelBridge::set_profiler_visible(bool visible) noexcept
    {
        profiler_visible_ = visible;
    }

    bool RuntimePanelBridge::profiler_visible() const noexcept
    {
        return profiler_visible_;
    }
} // namespace engine::tools::editor

