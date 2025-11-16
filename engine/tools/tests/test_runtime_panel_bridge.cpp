#include <gtest/gtest.h>

#include "engine/runtime/api.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/validation.hpp"
#include "engine/tools/editor/asset_browser_panel.hpp"
#include "engine/tools/editor/performance_metrics_panel.hpp"
#include "engine/tools/editor/runtime_panel_bridge.hpp"
#include "engine/tools/imgui/panel_registry.hpp"

namespace
{
    using engine::runtime::RuntimeDiagnostics;
    using engine::runtime::RuntimeStageTiming;
    using engine::scene::validation::HierarchyValidationReport;
    using engine::scene::Scene;
    using engine::tools::editor::AssetBrowserPanel;
    using engine::tools::editor::PerformanceMetricsPanel;
    using engine::tools::editor::RuntimePanelBridge;
    using engine::tools::imgui::PanelRegistry;
    using engine::tools::imgui::PanelRenderContext;
}

TEST(RuntimePanelBridge, RegistersDefaultPanelsAndRenders)
{
    PanelRegistry registry{};
    RuntimeDiagnostics diagnostics{};
    diagnostics.tick_count = 42;
    HierarchyValidationReport report{};
    report.metrics.issue_count = 7;

    bool diagnostics_rendered = false;
    bool profiler_rendered = false;
    bool scene_rendered = false;

    RuntimePanelBridge bridge(
        registry,
        [&]() -> const RuntimeDiagnostics& {
            diagnostics_rendered = true;
            return diagnostics;
        },
        [&]() -> const HierarchyValidationReport* {
            return &report;
        },
        RuntimePanelBridge::Renderers{
            [&](const RuntimeDiagnostics& value) {
                EXPECT_EQ(value.tick_count, diagnostics.tick_count);
            },
            [&](bool* visible) {
                ASSERT_NE(visible, nullptr);
                profiler_rendered = true;
                *visible = false;
            },
            [&](const HierarchyValidationReport& value) {
                scene_rendered = true;
                EXPECT_EQ(value.metrics.issue_count, report.metrics.issue_count);
            }
        }
    );

    bridge.render_all(0.016);

    EXPECT_TRUE(diagnostics_rendered);
    EXPECT_TRUE(profiler_rendered);
    EXPECT_TRUE(scene_rendered);
    EXPECT_FALSE(bridge.profiler_visible());

    bridge.set_profiler_visible(true);
    EXPECT_TRUE(bridge.profiler_visible());
}

TEST(RuntimePanelBridge, ForwardsRenderContextDeltaTime)
{
    PanelRegistry registry{};
    RuntimeDiagnostics diagnostics{};

    RuntimePanelBridge bridge(
        registry,
        [&]() -> const RuntimeDiagnostics& {
            return diagnostics;
        },
        RuntimePanelBridge::SceneValidationProvider{},
        RuntimePanelBridge::Renderers{
            [](const RuntimeDiagnostics&) {},
            [](bool*) {},
            [](const HierarchyValidationReport&) {}
        }
    );

    double observed_delta = -1.0;
    auto handle = registry.register_scoped_panel(
        "custom.panel",
        [&](const PanelRenderContext& ctx) {
            observed_delta = ctx.delta_time;
        }
    );
    ASSERT_TRUE(handle);

    bridge.render_all(0.033);
    EXPECT_DOUBLE_EQ(observed_delta, 0.033);
}

TEST(RuntimePanelBridge, ScenePanelSkipsWhenProviderReturnsNull)
{
    PanelRegistry registry{};
    RuntimeDiagnostics diagnostics{};

    bool scene_rendered = false;
    RuntimePanelBridge bridge(
        registry,
        [&]() -> const RuntimeDiagnostics& {
            return diagnostics;
        },
        [&]() -> const HierarchyValidationReport* {
            return nullptr;
        },
        RuntimePanelBridge::Renderers{
            [](const RuntimeDiagnostics&) {},
            [](bool*) {},
            [&](const HierarchyValidationReport&) {
                scene_rendered = true;
            }
        }
    );

    bridge.render_all(0.0);
    EXPECT_FALSE(scene_rendered);
}

TEST(RuntimePanelBridge, RegistersHierarchyPanelWhenHooksProvided)
{
    PanelRegistry registry{};
    RuntimeDiagnostics diagnostics{};
    Scene scene{};

    RuntimePanelBridge bridge(
        registry,
        [&]() -> const RuntimeDiagnostics& {
            return diagnostics;
        },
        RuntimePanelBridge::SceneValidationProvider{},
        RuntimePanelBridge::Renderers{
            [](const RuntimeDiagnostics&) {},
            [](bool*) {},
            [](const HierarchyValidationReport&) {}
        },
        RuntimePanelBridge::HierarchyPanelHooks{
            [&scene]() -> Scene* {
                return &scene;
            },
            RuntimePanelBridge::HierarchySelectionProvider{},
            RuntimePanelBridge::HierarchySelectionCallback{}
        }
    );

    EXPECT_TRUE(registry.contains("editor.scene_hierarchy"));

    EXPECT_NO_THROW(bridge.render_all(0.0));
}

TEST(RuntimePanelBridge, RegistersAssetBrowserWhenProviderAvailable)
{
    PanelRegistry registry{};
    RuntimeDiagnostics diagnostics{};

    std::size_t provider_invocations{0};

    RuntimePanelBridge bridge(
        registry,
        [&]() -> const RuntimeDiagnostics& {
            return diagnostics;
        },
        RuntimePanelBridge::SceneValidationProvider{},
        RuntimePanelBridge::Renderers{
            [](const RuntimeDiagnostics&) {},
            [](bool*) {},
            [](const HierarchyValidationReport&) {}
        },
        RuntimePanelBridge::HierarchyPanelHooks{},
        RuntimePanelBridge::AssetPanelHooks{
            [&]() {
                ++provider_invocations;
                std::vector<AssetBrowserPanel::AssetDescriptorRow> rows;
                rows.emplace_back(AssetBrowserPanel::AssetDescriptorRow{
                    .identifier = "mesh.asset",
                    .type = engine::assets::AssetType::mesh,
                    .status = "loaded",
                });
                return rows;
            }
        }
    );

    EXPECT_TRUE(registry.contains("editor.asset_browser"));

    bridge.render_all(0.016);
    EXPECT_GE(provider_invocations, 1U);
}

TEST(RuntimePanelBridge, RegistersPerformancePanelWhenDiagnosticsAvailable)
{
    PanelRegistry registry{};
    RuntimeDiagnostics diagnostics{};
    diagnostics.last_tick_ms = 5.0;
    diagnostics.average_tick_ms = 6.0;
    diagnostics.max_tick_ms = 8.0;
    diagnostics.stage_timings.push_back(RuntimeStageTiming{});
    diagnostics.stage_timings.back().name = "Stage";
    diagnostics.stage_timings.back().last_ms = 1.0;
    diagnostics.stage_timings.back().average_ms = 1.5;
    diagnostics.stage_timings.back().max_ms = 2.5;

    bool benchmark_invoked = false;
    RuntimePanelBridge::PerformancePanelHooks performance_hooks{};
    performance_hooks.history_capacity = 4;
    performance_hooks.benchmark_provider = [&]() {
        benchmark_invoked = true;
        std::vector<PerformanceMetricsPanel::BenchmarkEntry> entries;
        entries.emplace_back(PerformanceMetricsPanel::BenchmarkEntry{
            "demo",
            10.0,
            11.0,
        });
        return entries;
    };

    RuntimePanelBridge bridge(
        registry,
        [&]() -> const RuntimeDiagnostics& {
            return diagnostics;
        },
        RuntimePanelBridge::SceneValidationProvider{},
        RuntimePanelBridge::Renderers{
            [](const RuntimeDiagnostics&) {},
            [](bool*) {},
            [](const HierarchyValidationReport&) {},
        },
        RuntimePanelBridge::HierarchyPanelHooks{},
        RuntimePanelBridge::AssetPanelHooks{},
        performance_hooks
    );

    EXPECT_TRUE(registry.contains("runtime.performance_metrics"));

    bridge.render_all(0.0);
    EXPECT_TRUE(benchmark_invoked);
}

