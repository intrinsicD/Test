# Engine Tools Quick Reference

**Purpose:** Fast lookup for agents implementing tasks with engine/tools capabilities.

**Full Documentation:** `docs/modules/tools/README.md`  
**Integration Patterns:** `hybrid_workflow/CONTRIBUTING.md` §Diagnostic Tools & Performance

---

## When to Use Which Tool

| If You Need To... | Use This Tool | Header |
|-------------------|---------------|--------|
| **Profile performance** | `PROFILE_SCOPE` macro | `engine/tools/profiling/profiler.hpp` |
| **Visualize runtime diagnostics** | `render_diagnostics()` | `engine/tools/imgui_helpers.hpp` |
| **Show scene validation report** | `render_validation_report()` | `engine/tools/imgui_helpers.hpp` |
| **Display profiler data in UI** | `render_profiler_window()` | `engine/tools/imgui_helpers.hpp` |
| **Register reusable UI panels** | `PanelRegistry` | `engine/tools/imgui/panel_registry.hpp` |
| **Run headless benchmarks** | `PrototypeHarnessBenchmarkRunner` | `engine/tools/sandbox/benchmark_runner.hpp` |
| **Compare engine vs reference** | `ComparativeBenchmarkRunner` | `engine/tools/sandbox/benchmark_runner.hpp` |
| **Build prototyping UI** | `ExperimentSandbox` | `engine/tools/sandbox/experiment_sandbox.hpp` |
| **Load experiment configs** | `load_summary_from_json()` | `engine/tools/sandbox/configuration_loader.hpp` |

---

## Quick Examples

### 1. Profiling Performance-Critical Code

```cpp
#include "engine/tools/profiling/profiler.hpp"

void update_simulation(float dt) {
    // Automatic timing with RAII
    PROFILE_SCOPE("SimulationUpdate");
    
    physics_world.step(dt);
    animation_system.update(dt);
}

// Generate report
auto report = engine::tools::profiling::global_profiler().generate_report();
for (const auto& entry : report.entries) {
    fmt::print("{}: {:.3f}ms avg ({} calls)\n", 
               entry.name, entry.average_ms, entry.call_count);
}
```

**When:** Tasks with `perf` gate, optimization work, hot path analysis.

---

### 2. Rendering Runtime Diagnostics in ImGui

```cpp
#include "engine/tools/imgui_helpers.hpp"

void render_debug_ui() {
    engine::tools::imgui::begin_frame();
    
    if (ImGui::Begin("Diagnostics")) {
        // Automatic rendering of runtime metrics
        const auto& diag = runtime.diagnostics();
        engine::tools::imgui::render_diagnostics(diag);
    }
    ImGui::End();
    
    // Optional: show profiler window
    bool show_profiler = true;
    engine::tools::imgui::render_profiler_window(&show_profiler);
    
    engine::tools::imgui::end_frame();
}
```

**When:** Editor features, runtime inspection tools, debug overlays.

---

### 3. Registering Custom UI Panels

```cpp
#include "engine/tools/imgui/panel_registry.hpp"

using namespace engine::tools::imgui;

PanelRegistry registry;
auto my_panel = registry.register_scoped_panel(
    "my_panel",
    [](const PanelRenderContext& ctx) {
        ImGui::Text("Delta Time: %.2fms", ctx.delta_time * 1000.0);
        // ... custom UI code
    }
);

// Later: render all registered panels while the handle is alive
PanelRenderContext context{.delta_time = dt};
registry.render_all(context);
```

When wiring runtime telemetry, reuse the dedicated bridge so diagnostics, profiler, and scene validation panels register
automatically:

```cpp
engine::tools::editor::RuntimePanelBridge runtime_panels(
    registry,
    [&runtime]() -> const engine::runtime::RuntimeDiagnostics& { return runtime.diagnostics(); },
    [&runtime]() -> const engine::scene::validation::HierarchyValidationReport* {
        return &runtime.diagnostics().scene_validation;
    }
);
runtime_panels.render_all(dt);
```

**When:** Building editor with multiple panels, reusable diagnostic widgets. The returned `RegistrationHandle` automatically
unregisters the panel when it goes out of scope, keeping editor teardown deterministic.
**See:** TL-310 for editor foundation work.

---

### 4. Running Automated Benchmarks

```cpp
#include "engine/tools/sandbox/benchmark_runner.hpp"

using namespace engine::tools::sandbox;

// Headless benchmark via prototype harness
PrototypeHarnessBenchmarkRunner runner({
    "python", "-m", "scripts.prototyping.run_prototype_harness",
    "--config", config_path.string(),
}, summaries_dir);

SandboxPreferences prefs;
prefs.dataset = "remesh-sample";
prefs.rendering_preset = "research-baseline";
prefs.frame_count = 1000;
prefs.fixed_timestep = 1.0f / 60.0f;

auto result = runner.run(prefs);
if (result.success) {
    fmt::print("✓ {}\n{}\n", result.headline, result.details);
}
```

**When:** Automated performance testing, CI benchmarks, regression detection.

---

### 5. Loading Experiment Configurations

```cpp
#include "engine/tools/sandbox/configuration_loader.hpp"

// Load and validate experiment config
auto summary = engine::tools::sandbox::load_summary_from_json(
    "docs/examples/ai004_sample.json"
);

// Check datasets
fmt::print("Available datasets: {}\n", summary.datasets.size());
for (const auto& ds : summary.datasets) {
    fmt::print("  - {}: {} ({} assets)\n", 
               ds.identifier, ds.label, ds.assets.size());
    
    // Validate assets
    for (const auto& asset : ds.assets) {
        if (!asset.exists) {
            fmt::print("    ⚠ Missing: {}\n", asset.role);
        }
    }
}
```

**When:** Prototyping workflows, harness integration, dataset validation.

---

## Integration with Experiment Sandbox

For interactive prototyping workflows (AI-004):

```cpp
#include "engine/tools/sandbox/experiment_sandbox.hpp"
#include "engine/tools/sandbox/configuration_loader.hpp"
#include "engine/tools/sandbox/benchmark_runner.hpp"

using namespace engine::tools::sandbox;

// Load configuration
auto summary = load_summary_from_json(config_path);

// Create sandbox UI
ExperimentSandbox sandbox;
sandbox.set_configuration(summary);

// Wire callbacks
sandbox.set_callbacks({
    .on_dataset_selected = [](const std::string& id) {
        fmt::print("Dataset changed: {}\n", id);
        // Update harness...
    },
    .on_rendering_changed = [](const SandboxPreferences& prefs) {
        fmt::print("Rendering: preset={}, shading={}\n",
                   prefs.rendering_preset, prefs.shading_mode);
        // Update rendering config...
    },
    .on_run_benchmark = [&](const SandboxPreferences& prefs) {
        return runner.run(prefs);
    },
});

// Programmatic control (optional)
sandbox.select_dataset("remesh-sample");
sandbox.select_rendering_preset("research-baseline");
sandbox.set_shading_mode("deferred");
sandbox.set_overlay_enabled("normals", true);

// Render UI
sandbox.render();

// Trigger benchmark programmatically
auto result = sandbox.run_active_benchmark();
```

**When:** AI-004 prototyping, interactive dataset testing, comparative workflows.

---

## Tool Selection by Task Type

### Performance Optimization Tasks (perf gate)
✅ **Use:**
- `PROFILE_SCOPE` for timing hot paths
- `PrototypeHarnessBenchmarkRunner` for before/after comparison
- `ComparativeBenchmarkRunner` for reference validation

📝 **Evidence:** Include profiler report and benchmark logs in task Evidence section.

### Editor/UI Feature Tasks
✅ **Use:**
- `render_diagnostics()` for runtime state visualization
- `PanelRegistry` for reusable panels
- `render_profiler_window()` for performance overlay

📝 **Evidence:** Screenshot panels, document panel registration.

### Prototyping Workflow Tasks (AI-004)
✅ **Use:**
- `ExperimentSandbox` for interactive UI
- `load_summary_from_json()` for configuration
- `PrototypeHarnessBenchmarkRunner` for automation

📝 **Evidence:** Config validation output, sandbox screenshots.

### Scene/Runtime Validation Tasks
✅ **Use:**
- `render_validation_report()` for hierarchy checks
- `render_diagnostics()` for telemetry inspection
- `load_summary_from_json()` for test scenario configs

📝 **Evidence:** Validation reports, diagnostic screenshots.

---

## Common Patterns

### Pattern: Profile → Optimize → Benchmark

```cpp
// 1. Profile baseline
{
    PROFILE_SCOPE("OriginalImplementation");
    original_function();
}

// 2. Implement optimization
{
    PROFILE_SCOPE("OptimizedImplementation");
    optimized_function();
}

// 3. Generate report
auto report = global_profiler().generate_report();

// 4. Run automated benchmark
PrototypeHarnessBenchmarkRunner runner(/* ... */);
auto baseline = runner.run(baseline_prefs);
auto optimized = runner.run(optimized_prefs);

// 5. Compare and document in Evidence section
```

### Pattern: Configuration → Validation → Execution

```cpp
// 1. Load config
auto summary = load_summary_from_json(config_path);

// 2. Validate assets
bool all_valid = true;
for (const auto& ds : summary.datasets) {
    for (const auto& asset : ds.assets) {
        if (!asset.exists || !asset.verified) {
            fmt::print("Invalid: {}\n", asset.path);
            all_valid = false;
        }
    }
}

// 3. Execute if valid
if (all_valid) {
    ExperimentSandbox sandbox;
    sandbox.set_configuration(summary);
    // ... proceed
}
```

### Pattern: Panel Registration for Editor

```cpp
// In editor initialization
PanelRegistry registry;

// Register standard panels and keep the handles alive for the editor lifetime
auto scene_panel = registry.register_scoped_panel("scene", render_scene_panel);
auto inspector_panel = registry.register_scoped_panel("inspector", render_inspector_panel);
auto profiler_panel = registry.register_scoped_panel("profiler", [](const auto& ctx) {
    engine::tools::imgui::render_profiler_window();
});
auto diagnostics_panel = registry.register_scoped_panel("diagnostics", [&](const auto& ctx) {
    engine::tools::imgui::render_diagnostics(runtime.diagnostics());
});

// In editor loop
PanelRenderContext ctx{.delta_time = dt};
registry.render_all(ctx);
```

---

## Troubleshooting

### Profiler shows zero timings
- **Cause:** Forgot to call `begin()` / `end()` or mismatched names
- **Fix:** Use `PROFILE_SCOPE` for automatic RAII cleanup

### Benchmark runner fails
- **Cause:** Python harness not in PATH, config file missing
- **Fix:** Verify command prefix, check config path exists

### Configuration loader throws exception
- **Cause:** JSON schema mismatch, malformed file
- **Fix:** Validate JSON against harness `--describe-json` output

### Panels not rendering
- **Cause:** Missing `begin_frame()` / `end_frame()` calls
- **Fix:** Wrap ImGui code in `imgui::begin_frame()` / `imgui::end_frame()`

### Panel registry duplicate identifier
- **Cause:** Attempted to register same identifier twice
- **Fix:** Use unique identifiers or unregister before re-registering

---

## References

- **Full Documentation:** `docs/modules/tools/README.md`
- **Integration Guide:** `hybrid_workflow/CONTRIBUTING.md` §Diagnostic Tools
- **Tools Analysis:** `hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md`
- **Task Template:** `hybrid_workflow/backlog/000-template.md` §Tool Integration
- **Editor Foundations:** `hybrid_workflow/backlog/TL-310-editor-foundations.md`
- **Experiment Design:** `docs/design/TL_210_EXPERIMENT_SANDBOX.md`
- **ADR-0008:** `docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`

---

**Last Updated:** 2025-11-06  
**Maintainer:** Tools Lead / Agent Orchestrator

