#include "engine/tools/sandbox/experiment_sandbox.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <system_error>

using namespace engine::tools::sandbox;

namespace
{
    ExperimentConfigurationSummary make_summary()
    {
        ExperimentConfigurationSummary summary;

        DatasetDescriptor dataset_a{};
        dataset_a.identifier = "dataset_a";
        dataset_a.label = "Dataset A";
        dataset_a.kind = "remesh";
        dataset_a.tags = {"baseline", "geometry"};
        dataset_a.statistics = {{"faces", 4200.0}, {"vertices", 2150.0}};
        dataset_a.metrics = {{"mean_edge", 0.82}};
        dataset_a.source_asset = "assets/raw/a.obj";
        dataset_a.processed_asset = "assets/processed/a.mesh";

        DatasetDescriptor dataset_b{};
        dataset_b.identifier = "dataset_b";
        dataset_b.kind = "parameterization";
        dataset_b.tags = {"uv", "case-study"};

        summary.datasets = {dataset_a, dataset_b};
        summary.selected_dataset = dataset_a.identifier;

        RenderingPresetDescriptor preset{};
        preset.identifier = "research";
        preset.label = "Research Baseline";
        preset.shading_modes = {"Forward", "Deferred"};
        preset.overlays = {
            OverlayDescriptor{.key = "normals", .label = "Show normals", .default_enabled = false},
            OverlayDescriptor{.key = "uv", .label = "Show UVs", .default_enabled = false},
        };

        summary.rendering_presets = {preset};

        RuntimeSummary runtime{};
        runtime.dataset_identifier = dataset_a.identifier;
        runtime.scene_manifest = "docs/examples/ai004_sample.json";
        runtime.scene_entry_point = "geometry_viewer";
        runtime.camera_description = "Orbit camera (1.5m radius)";
        runtime.simulation_description = "Static showcase";
        runtime.hot_reload_enabled = true;

        summary.runtime = runtime;
        return summary;
    }
}

TEST(ExperimentSandbox, DefaultsToSummarySelection)
{
    ExperimentSandbox sandbox;
    auto summary = make_summary();
    sandbox.set_configuration(summary);

    const auto& prefs = sandbox.preferences();
    EXPECT_EQ(prefs.selected_dataset, "dataset_a");
    EXPECT_EQ(prefs.selected_preset, "research");
    EXPECT_EQ(prefs.shading_mode, "Forward");
}

TEST(ExperimentSandbox, PreferenceRoundTrip)
{
    ExperimentSandbox sandbox;
    const auto summary = make_summary();
    sandbox.set_configuration(summary);

    SandboxPreferences preferences;
    preferences.selected_dataset = "dataset_b";
    preferences.selected_preset = "research";
    preferences.shading_mode = "Deferred";
    preferences.overlays = {{"normals", true}, {"uv", false}};
    preferences.benchmark_frames = 480;
    preferences.benchmark_timestep = 0.01F;
    sandbox.set_preferences(preferences);

    const auto temp_path = std::filesystem::temp_directory_path() / "experiment_sandbox_prefs.ini";
    ASSERT_TRUE(sandbox.save_preferences(temp_path));

    ExperimentSandbox loaded;
    loaded.set_configuration(summary);
    ASSERT_TRUE(loaded.load_preferences(temp_path));

    const auto& loaded_prefs = loaded.preferences();
    EXPECT_EQ(loaded_prefs.selected_dataset, preferences.selected_dataset);
    EXPECT_EQ(loaded_prefs.selected_preset, preferences.selected_preset);
    EXPECT_EQ(loaded_prefs.shading_mode, preferences.shading_mode);
    ASSERT_TRUE(loaded_prefs.overlays.contains("normals"));
    EXPECT_TRUE(loaded_prefs.overlays.at("normals"));
    ASSERT_TRUE(loaded_prefs.overlays.contains("uv"));
    EXPECT_FALSE(loaded_prefs.overlays.at("uv"));
    EXPECT_EQ(loaded_prefs.benchmark_frames, preferences.benchmark_frames);
    EXPECT_FLOAT_EQ(loaded_prefs.benchmark_timestep, preferences.benchmark_timestep);

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

TEST(ExperimentSandbox, RecordsBenchmarkResult)
{
    ExperimentSandbox sandbox;
    SandboxBenchmarkResult result{};
    result.success = true;
    result.headline = "Benchmark succeeded";
    result.details = "frames=600";
    sandbox.apply_benchmark_result(result);

    const auto& stored = sandbox.last_benchmark_result();
    ASSERT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->success);
    EXPECT_EQ(stored->headline, result.headline);
    EXPECT_EQ(stored->details, result.details);
}

TEST(ExperimentSandbox, DatasetSelectionInvokesCallback)
{
    ExperimentSandbox sandbox;
    auto summary = make_summary();
    summary.selected_dataset = summary.datasets.front().identifier;

    std::string last_dataset;
    int callback_count = 0;
    SandboxCallbacks callbacks{};
    callbacks.on_dataset_selected = [&](const std::string& identifier) {
        ++callback_count;
        last_dataset = identifier;
    };

    sandbox.set_configuration(summary);
    sandbox.set_callbacks(callbacks);

    ASSERT_TRUE(sandbox.select_dataset("dataset_b"));
    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(last_dataset, "dataset_b");
    EXPECT_EQ(sandbox.preferences().selected_dataset, "dataset_b");

    EXPECT_FALSE(sandbox.select_dataset("unknown"));
    EXPECT_EQ(callback_count, 1) << "Callback should not trigger for invalid dataset";
}

TEST(ExperimentSandbox, RenderingSelectionInvokesCallback)
{
    ExperimentSandbox sandbox;
    auto summary = make_summary();

    RenderingPresetDescriptor secondary{};
    secondary.identifier = "diagnostics";
    secondary.label = "Diagnostics";
    secondary.shading_modes = {"Forward"};
    secondary.overlays = {
        OverlayDescriptor{.key = "wireframe", .label = "Wireframe", .default_enabled = true},
    };
    summary.rendering_presets.push_back(secondary);

    SandboxPreferences captured{};
    int render_callback_count = 0;
    SandboxCallbacks callbacks{};
    callbacks.on_rendering_changed = [&](const SandboxPreferences& prefs) {
        ++render_callback_count;
        captured = prefs;
    };

    sandbox.set_configuration(summary);
    sandbox.set_callbacks(callbacks);

    ASSERT_TRUE(sandbox.select_rendering_preset("diagnostics"));
    EXPECT_GE(render_callback_count, 1);
    EXPECT_EQ(captured.selected_preset, "diagnostics");
    ASSERT_TRUE(captured.overlays.contains("wireframe"));
    EXPECT_TRUE(captured.overlays.at("wireframe"));

    const int callbacks_after_preset = render_callback_count;
    EXPECT_TRUE(sandbox.set_shading_mode("Forward"));
    EXPECT_EQ(sandbox.preferences().shading_mode, "Forward");
    EXPECT_EQ(render_callback_count, callbacks_after_preset)
        << "Selecting the existing shading mode should not trigger callbacks";

    EXPECT_FALSE(sandbox.set_shading_mode("Deferred"));
    EXPECT_EQ(render_callback_count, callbacks_after_preset);
}

TEST(ExperimentSandbox, OverlayToggleInvokesCallback)
{
    ExperimentSandbox sandbox;
    auto summary = make_summary();

    SandboxPreferences captured{};
    int render_callback_count = 0;
    SandboxCallbacks callbacks{};
    callbacks.on_rendering_changed = [&](const SandboxPreferences& prefs) {
        ++render_callback_count;
        captured = prefs;
    };

    sandbox.set_configuration(summary);
    sandbox.set_callbacks(callbacks);

    ASSERT_TRUE(sandbox.set_overlay_enabled("normals", true));
    EXPECT_EQ(render_callback_count, 1);
    ASSERT_TRUE(captured.overlays.contains("normals"));
    EXPECT_TRUE(captured.overlays.at("normals"));

    EXPECT_FALSE(sandbox.set_overlay_enabled("unknown", true));
    EXPECT_EQ(render_callback_count, 1) << "Unknown overlays should not trigger callbacks";
}

