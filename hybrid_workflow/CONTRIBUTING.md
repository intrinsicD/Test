# Contributing

## Code Style

- Language: C++20; treat warnings as errors across presets.
- Formatting: `clang-format` (see repository `.clang-format`).
- Lint: `clang-tidy` (wired through CI smoke jobs).
- Files: `snake_case` filenames; `PascalCase` types; `camelCase` methods and variables.

## Naming Conventions

| Concern                    | Convention                                                    | Example                                             |
|----------------------------|---------------------------------------------------------------|-----------------------------------------------------|
| C++ Types / Classes        | `PascalCase`                                                  | `SurfaceMesh`, `RuntimeHost`                        |
| C++ Functions              | `PascalCase` for APIs, `snake_case` for internal helpers      | `CreateSceneGraph`, `build_registry()`              |
| C++ Variables              | `snake_case`                                                  | `vertex_count`, `frame_timer`                       |
| C++ Constants & Macros     | `UPPER_SNAKE_CASE`                                            | `MAX_ITERATIONS`, `ENGINE_RUNTIME_EXPORTS`          |
| Namespaces                 | lower-case, scoped by module                                  | `engine::scene`, `engine::geometry`                 |
| Python Modules & Functions | `snake_case`                                                  | `load_scene`, `compute_normals`                     |
| Python Classes             | `PascalCase`                                                  | `MeshLoader`, `TelemetryReport`                     |
| Python Constants           | `UPPER_SNAKE_CASE`                                            | `DEFAULT_TIMEOUT`                                   |
| Markdown files             | `UPPER_SNAKE_CASE`                                            | `README.md`, `CONTRIBUTING.md`                      |
| Files & Directories        | lower-case with hyphens/underscores unless part of public API | `engine/scene/systems`, `python/engine3g/loader.py` |

## Project Layout

- Public headers → `engine/<module>/include/engine/<module>/`
- Sources → `engine/<module>/src/`
- Tests → `engine/<module>/tests/` + `python/tests/`, `scripts/tests/`
- Benchmarks → `engine/<module>/bench/`
- Demos/Tools → `engine/<module>/samples/`, `tools/examples/`

## C++ Guidelines

- Target **C++20**; avoid non-standard extensions.
- Use non-owning views (`std::string_view`, `std::span`) for shared data.
- Guard headers with `#pragma once` and minimal includes.
- Propagate recoverable failures with `engine::Result<T, ErrorCode>`; never throw from public API entry points.
- Wire new targets through existing module helpers (`engine_apply_module_defaults`) and keep CMake presets updated.

## Python Guidelines

- Require **Python 3.12+** with type annotations for public APIs.
- Structure reusable logic under `python/engine3g/` and curate exports in `__init__.py`.
- Prefer `pathlib.Path` over raw strings and keep imports side-effect free.
- Document public functions/classes with docstrings describing arguments, return values, and error semantics.

## Data-Oriented Defaults

- Prefer **SoA** in hot paths; avoid virtual dispatch inside tight loops.
- No global singletons; pass explicit context/handles.
- Keep subsystem interfaces narrow and deterministic.

## Build & Tooling

- CMake ≥ 3.20; use presets from `CMakePresets.json` / `scripts/build/presets/`.
- Optional SDKs: Vulkan SDK 1.3.x, CUDA toolkit, GLFW desktop dependencies.
- Profiling: Capture Tracy zones around profiled hot paths and commit configs alongside code.

## Testing

- Framework: GoogleTest + CTest orchestration; Python uses `pytest`.
- Coverage: target ≥80% where instrumentation exists; add regression coverage for every bug fix.
- Every public API change must add/adjust tests across native and Python bindings.

## Diagnostic Tools & ImGui Integration

For runtime diagnostics and debug UI, use the tools module helpers:

### ImGui Diagnostics Rendering

```cpp
#include "engine/tools/imgui_helpers.hpp"
#include "engine/runtime/diagnostics.hpp"
#include "engine/scene/validation.hpp"

// In main loop or editor update
engine::tools::imgui::begin_frame();

// Runtime diagnostics panel
if (ImGui::Begin("Runtime Diagnostics")) {
    const auto& diag = runtime.diagnostics();
    engine::tools::imgui::render_diagnostics(diag);
}
ImGui::End();

// Scene validation report
if (ImGui::Begin("Scene Validation")) {
    const auto& report = scene.validate_hierarchy();
    engine::tools::imgui::render_validation_report(report);
}
ImGui::End();

// Profiler visualization
bool show_profiler = true;
engine::tools::imgui::render_profiler_window(&show_profiler);

engine::tools::imgui::end_frame();
```

### Panel Registry (for Editor Components)

Register reusable diagnostic panels:

```cpp
#include "engine/tools/imgui/panel_registry.hpp"

using namespace engine::tools::imgui;

PanelRegistry registry;

// Register custom panels and retain the handles until teardown
auto scene_hierarchy_panel = registry.register_scoped_panel(
    "scene_hierarchy",
    [](const PanelRenderContext& ctx) {
        ImGui::Text("Scene Graph");
        // ... render scene hierarchy
    }
);

auto performance_panel = registry.register_scoped_panel(
    "performance_metrics",
    [](const PanelRenderContext& ctx) {
        ImGui::Text("Frame Time: %.2fms", ctx.delta_time * 1000.0);
        // ... render performance graphs
    }
);

// Render all panels while the handles remain valid
PanelRenderContext context{.delta_time = dt};
if (scene_hierarchy_panel && performance_panel)
{
    registry.render_all(context);

    // Or render a specific panel
    registry.render("scene_hierarchy", context);
}
```

**When to Use:**
- Use `render_diagnostics()` for quick runtime inspection
- Use `render_validation_report()` for scene health checks
- Use `render_profiler_window()` to visualize performance data
- Use `PanelRegistry` when building editor or tool surfaces (see TL-310); prefer `register_scoped_panel()` so panels unregister
  automatically when the owning subsystem shuts down.

### Configuration Loading

Parse experiment configurations for prototyping workflows:

```cpp
#include "engine/tools/sandbox/configuration_loader.hpp"

// Load from JSON file
auto summary = engine::tools::sandbox::load_summary_from_json(
    "docs/examples/ai004_sample.json"
);

// Access configuration data
fmt::print("Datasets: {}\n", summary.datasets.size());
for (const auto& ds : summary.datasets) {
    fmt::print("  - {}: {} ({} assets)\n", 
               ds.identifier, ds.label, ds.assets.size());
}

// Validate dataset assets
for (const auto& asset : summary.datasets[0].assets) {
    if (!asset.exists) {
        fmt::print("Missing: {} ({})\n", asset.role, asset.path);
    }
    if (asset.expected_sha256 && asset.actual_sha256 &&
        *asset.expected_sha256 != *asset.actual_sha256) {
        fmt::print("Hash mismatch: {}\n", asset.role);
    }
}
```

## Performance

- Framework: GoogleBenchmark and bespoke telemetry harnesses.
- Regressions > **2%** block merges unless waived with mitigation in task file + quality report.
- Store raw results in task evidence sections or `telemetry/`; link in PR descriptions.

### Profiler Integration

Use the engine profiler for performance-critical code:

```cpp
#include "engine/tools/profiling/profiler.hpp"

// Scoped profiling (automatic timing)
void update_physics(float dt) {
    PROFILE_SCOPE("PhysicsUpdate");
    world.step(dt);
}

// Manual profiling (fine-grained control)
void complex_operation() {
    auto& profiler = engine::tools::profiling::global_profiler();
    
    profiler.begin("LoadAssets");
    load_scene_assets();
    profiler.end("LoadAssets");
    
    profiler.begin("BuildAcceleration");
    build_spatial_index();
    profiler.end("BuildAcceleration");
}

// Generate evidence report
auto report = engine::tools::profiling::global_profiler().generate_report();
for (const auto& entry : report.entries) {
    fmt::print("{}: avg={:.3f}ms, min={:.3f}ms, max={:.3f}ms, calls={}\n",
               entry.name, entry.average_ms, entry.min_ms, entry.max_ms, entry.call_count);
}
```

**Best Practices:**
- Use `PROFILE_SCOPE` for automatic cleanup in RAII style
- Profile at function/system boundaries, not inside tight loops
- Reset profiler between benchmark runs: `global_profiler().reset()`
- Include profiler output in task evidence for `perf` gate

### Benchmark Automation

For automated benchmark execution, use the benchmark runners:

```cpp
#include "engine/tools/sandbox/benchmark_runner.hpp"

using namespace engine::tools::sandbox;

// Prototype harness benchmark (headless)
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
    fmt::print("Benchmark: {}\n{}\n", result.headline, result.details);
} else {
    fmt::print("Failed: {}\n{}\n", result.headline, result.details);
}

// Comparative benchmarks (engine vs reference)
ComparativeBenchmarkRunner comparative_runner({
    "python", 
    (project_root / "scripts/benchmarks/run_comparative_benchmarks.py").string(),
}, working_dir);

BenchmarkScenarioDescriptor scenario = /* load from config */;
auto comparative_result = comparative_runner.run(scenario, prefs);
```

**Integration with Sandbox:**

```cpp
#include "engine/tools/sandbox/experiment_sandbox.hpp"

ExperimentSandbox sandbox;
sandbox.set_configuration(load_summary_from_json(config_path));

// Wire benchmark runner
sandbox.set_callbacks({
    .on_run_benchmark = [&](const SandboxPreferences& prefs) {
        return runner.run(prefs);
    },
});

// Programmatic benchmark execution
auto result = sandbox.run_active_benchmark();
```

## Git & PR Rules

- One PR per task; reference the **`hybrid_workflow/backlog/...`** file path in PR body.
- Branch naming: `feat|fix|refactor/NNN-kebab-title` aligned with backlog IDs.
- Keep diffs <400 LOC; split otherwise.
- Commit messages: imperative mood; mention roadmap/backlog IDs.

## Documentation

- Update **hybrid_workflow/README.md** and root docs if user-facing behavior changes.
- API changes → refresh module READMEs under `docs/modules/` and navigation tables.
- Run `python scripts/validate_docs.py` after doc edits to keep cross-links green.
- Keep task files updated with design decisions and evidence as work progresses.

## Security & Safety

- For file I/O, plugins, scripting, or network changes: follow safety checklist in the task file.
- Validate untrusted inputs and document threat model updates in ADRs or module docs.
- Run sanitizers when `safety` gate is specified: `cmake --preset linux-clang-debug-asan`

## CI Expectations

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

## Error Handling

- Follow [`docs/design/ERROR_HANDLING_MIGRATION.md`](../docs/design/ERROR_HANDLING_MIGRATION.md) for status/result patterns.
- Prefer `engine::Result<T, ErrorCode>` over exceptions in public APIs.
- Document error conditions in function/class headers.

## Review Checklist

Before requesting review, verify:

- [ ] Code conforms to naming and formatting conventions above.
- [ ] Tests cover new behavior and run cleanly via canonical build workflow.
- [ ] Documentation updates are complete (module READMEs, task file, cross-links validated).
- [ ] Architectural invariants remain intact or changes documented in ADRs.
- [ ] Task frontmatter reflects current status and all gates are addressed.
- [ ] Performance benchmarks captured if `perf` gate specified.
- [ ] Profiler evidence included for performance-critical code (if `perf` gate).
- [ ] Diagnostic panels registered if adding editor/UI components (if applicable).
- [ ] Benchmark automation used for comparative performance validation (if applicable).
- [ ] Configuration validation performed for prototyping workflows (if applicable).
- [ ] Risks and deviations noted in task file and PR description.

---

> See **[AGENTS.md](./AGENTS.md)** for the full workflow and task lifecycle.

