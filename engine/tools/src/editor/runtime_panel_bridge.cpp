#include "engine/tools/editor/runtime_panel_bridge.hpp"

#include "engine/tools/imgui_helpers.hpp"
#include "engine/tools/profiling/profiler.hpp"

#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "engine/core/telemetry/schema.hpp"
#include "engine/runtime/api.hpp"
#include "engine/scene/validation.hpp"

namespace engine::tools::editor
{
    namespace
    {
        using core::telemetry::MetricSet;

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

        [[nodiscard]] std::vector<TelemetryVisualizationPanel::SeriesSample> make_metric_samples(
            const MetricSet& metric_set
        )
        {
            std::vector<TelemetryVisualizationPanel::SeriesSample> samples;
            if (metric_set.samples.empty() || metric_set.descriptors.empty())
            {
                return samples;
            }

            samples.reserve(metric_set.samples.size());
            for (const auto& sample : metric_set.samples)
            {
                if (sample.descriptor_index >= metric_set.descriptors.size())
                {
                    spdlog::warn(
                        "Telemetry sample referenced descriptor {} but only {} descriptors are available; skipping.",
                        sample.descriptor_index,
                        metric_set.descriptors.size()
                    );
                    continue;
                }

                const auto& descriptor = metric_set.descriptors[sample.descriptor_index];
                TelemetryVisualizationPanel::SeriesSample entry{};
                entry.identifier = descriptor.name;
                entry.label = descriptor.name;
                entry.unit = std::string(core::telemetry::to_string(descriptor.unit));
                entry.value = core::telemetry::as_double(sample.value);
                samples.push_back(std::move(entry));
            }

            return samples;
        }
    } // namespace

    RuntimePanelBridge::RuntimePanelBridge(
        imgui::PanelRegistry& registry,
        DiagnosticsProvider diagnostics_provider,
        SceneValidationProvider scene_validation_provider,
        Renderers renderers,
        HierarchyPanelHooks hierarchy_hooks,
        AssetPanelHooks asset_hooks,
        PerformancePanelHooks performance_hooks,
        TelemetryPanelHooks telemetry_hooks
    )
        : registry_(&registry)
        , diagnostics_provider_(std::move(diagnostics_provider))
        , scene_validation_provider_(std::move(scene_validation_provider))
        , renderers_(std::move(renderers))
        , hierarchy_hooks_(std::move(hierarchy_hooks))
        , asset_hooks_(std::move(asset_hooks))
        , performance_hooks_(std::move(performance_hooks))
        , telemetry_hooks_(std::move(telemetry_hooks))
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

        if (registry_ && asset_hooks_.row_provider)
        {
            asset_panel_ = std::make_unique<AssetBrowserPanel>();
            asset_handle_ = registry_->register_scoped_panel(
                "editor.asset_browser",
                [this](const imgui::PanelRenderContext& context) {
                    if (!asset_panel_)
                    {
                        return;
                    }

                    auto rows = asset_hooks_.row_provider();
                    asset_panel_->set_rows(std::move(rows));
                    asset_panel_->render(context);
                }
            );
        }

        if (registry_ && diagnostics_provider_)
        {
            performance_panel_ = std::make_unique<PerformanceMetricsPanel>();
            performance_panel_->set_history_capacity(performance_hooks_.history_capacity);
            performance_handle_ = registry_->register_scoped_panel(
                "runtime.performance_metrics",
                [this](const imgui::PanelRenderContext& context) {
                    if (performance_panel_)
                    {
                        performance_panel_->render(context);
                    }
                }
            );
        }

        if (registry_ && diagnostics_provider_)
        {
            telemetry_panel_ = std::make_unique<TelemetryVisualizationPanel>();
            telemetry_panel_->set_history_capacity(telemetry_hooks_.history_capacity);
            telemetry_handle_ = registry_->register_scoped_panel(
                "runtime.telemetry",
                [this](const imgui::PanelRenderContext& context) {
                    if (telemetry_panel_)
                    {
                        telemetry_panel_->render(context);
                    }
                }
            );
        }
    }

    void RuntimePanelBridge::render_all(double delta_time) const
    {
        if (!registry_)
        {
            return;
        }

        synchronize_panel_state();

        const imgui::PanelRenderContext context{.delta_time = delta_time};
        registry_->render_all(context);
    }

    void RuntimePanelBridge::render_panels(
        double delta_time,
        const std::vector<std::string_view>& identifiers
    ) const
    {
        if (!registry_)
        {
            return;
        }

        synchronize_panel_state();

        if (identifiers.empty())
        {
            return;
        }

        const imgui::PanelRenderContext context{.delta_time = delta_time};
        for (std::string_view identifier : identifiers)
        {
            registry_->render(identifier, context);
        }
    }

    std::vector<std::string> RuntimePanelBridge::panel_identifiers() const
    {
        if (!registry_)
        {
            return {};
        }

        return registry_->identifiers();
    }

    void RuntimePanelBridge::synchronize_panel_state() const
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

        if ((performance_panel_ || telemetry_panel_) && diagnostics_provider_)
        {
            const auto& diagnostics = diagnostics_provider_();
            if (performance_panel_)
            {
                PerformanceMetricsPanel::FrameSample sample{
                    diagnostics.last_tick_ms,
                    diagnostics.average_tick_ms,
                    diagnostics.max_tick_ms,
                };
                performance_panel_->push_frame_sample(sample);

                std::vector<PerformanceMetricsPanel::StageTimingRow> stage_rows;
                stage_rows.reserve(diagnostics.stage_timings.size());
                for (const auto& stage : diagnostics.stage_timings)
                {
                    stage_rows.push_back(PerformanceMetricsPanel::StageTimingRow{
                        stage.name,
                        stage.last_ms,
                        stage.average_ms,
                        stage.max_ms,
                    });
                }
                performance_panel_->set_stage_timings(std::move(stage_rows));

                const auto profiler_report = profiling::global_profiler().generate_report();
                std::vector<PerformanceMetricsPanel::ProfilerEntryRow> profiler_rows;
                profiler_rows.reserve(profiler_report.entries.size());
                for (const auto& entry : profiler_report.entries)
                {
                    profiler_rows.push_back(PerformanceMetricsPanel::ProfilerEntryRow{
                        entry.name,
                        entry.duration_ms,
                        entry.average_ms,
                        entry.min_ms,
                        entry.max_ms,
                        entry.call_count,
                    });
                }
                performance_panel_->set_profiler_entries(std::move(profiler_rows));

                if (performance_hooks_.benchmark_provider)
                {
                    performance_panel_->set_benchmark_entries(performance_hooks_.benchmark_provider());
                }
                else
                {
                    performance_panel_->set_benchmark_entries({});
                }
            }

            if (telemetry_panel_)
            {
                std::vector<TelemetryVisualizationPanel::SeriesSample> samples;
                if (telemetry_hooks_.series_provider)
                {
                    samples = telemetry_hooks_.series_provider(diagnostics);
                }
                else
                {
                    samples = make_metric_samples(diagnostics.metrics);
                }

                telemetry_panel_->update_series(std::move(samples));
            }
        }
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

