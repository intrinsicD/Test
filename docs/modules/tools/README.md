# Tools Module

## Overview

The tools module provides editor utilities, profiling tools, pipeline automation, and diagnostics viewers. It includes the telemetry viewer CLI for runtime snapshots, integration with Dear ImGui for debug UI, and runtime packaging scripts for CI/CD workflows.

**Status:** ✅ **Modularization Complete** - The tools module has been fully modularized and is now enabled in the build system.

## Telemetry Viewer CLI

Command-line tool for capturing and analyzing runtime diagnostics:

```bash
# Capture telemetry snapshot
python scripts/diagnostics/runtime_frame_telemetry.py \
    --library-dir build/lib \
    --output telemetry_snapshot.json

# View telemetry report
python scripts/diagnostics/telemetry_viewer.py telemetry_snapshot.json
```

Features:
- **Lifecycle metrics**: Initialize/tick/shutdown counts and timings
- **Streaming health**: Async queue metrics, completion/failure rates
- **Scene validation**: Cycle detection, depth analysis, alert levels
- **Physics telemetry**: Collision metrics, manifold cache statistics
- **Handle validation**: Asset and rendering handle lifecycle tracking

The viewer integrates with the telemetry schema from `CC-001`:

```python
from engine3g import telemetry

# Load snapshot
snapshot = telemetry.load_snapshot("telemetry_snapshot.json")

# Query metrics
tick_count = snapshot.get_counter("runtime.lifecycle.tick")
avg_tick_ms = snapshot.get_histogram("runtime.tick_ms").average

print(f"Ticks: {tick_count}, Avg: {avg_tick_ms:.3f}ms")
```

## Dear ImGui Integration

Debug UI for runtime inspection:

```cpp
#include "engine/tools/imgui_helpers.hpp"

// In main loop
tools::imgui::begin_frame();

if (ImGui::Begin("Runtime Diagnostics")) {
    const auto& diag = runtime::diagnostics();
    
    ImGui::Text("Tick: %llu", diag.tick_count);
    ImGui::Text("Avg: %.3fms", diag.average_tick_ms);
    
    if (ImGui::CollapsingHeader("Streaming")) {
        ImGui::Text("Pending: %zu", diag.streaming.streaming_pending);
        ImGui::Text("Completed: %llu", diag.streaming.streaming_total_completed);
        ImGui::Text("Failed: %llu", diag.streaming.streaming_total_failed);
    }
    
    if (ImGui::CollapsingHeader("Scene Validation")) {
        tools::imgui::render_validation_report(diag.scene_validation);
    }
}
ImGui::End();

tools::imgui::end_frame();
```

## Runtime Packaging

Automate runtime artifact packaging for distribution:

```bash
# Package runtime artifacts
python scripts/ci/package_runtime_artifacts.py \
    --build-dir build \
    --output artifacts/ \
    --platform linux-x64

# Generates:
# - artifacts/libengine_runtime.so
# - artifacts/manifest.json (with checksums)
# - artifacts/telemetry_schema.json
```

The packaging script:
- Collects runtime libraries and dependencies
- Generates manifest with version and checksums
- Includes telemetry schema for tooling compatibility
- Validates artifact completeness

Integrated into CI pipelines to ensure reproducible builds.

## Profiling Utilities

Performance profiling helpers:

```cpp
#include "engine/tools/profiling/profiler.hpp"

// Scoped profiling
{
    PROFILE_SCOPE("PhysicsUpdate");
    world.step(dt);
}

// Manual profiling
tools::profiling::Profiler profiler;
profiler.begin("RenderSubmission");
submit_frame_graph();
profiler.end("RenderSubmission");

// Generate report
auto report = profiler.generate_report();
for (const auto& entry : report.entries) {
    fmt::print("{}: {:.3f}ms\n", entry.name, entry.duration_ms);
}
```

## Pipeline Automation

Tools for asset pipeline automation:

```bash
# Process asset directory
python scripts/tools/process_assets.py \
    --input assets/raw/ \
    --output assets/processed/ \
    --formats obj,stl,ply

# Validate asset integrity
python scripts/tools/validate_assets.py assets/processed/
```

Asset processor features:
- Format conversion (OBJ → custom binary format)
- Mesh optimization (vertex cache, overdraw reduction)
- Texture compression
- Validation and checksumming

## Editor (Planned)

Future editor application built on the tools module:

- **Scene editor**: Visual scene graph editing with ImGui
- **Material editor**: Shader graph authoring
- **Animation editor**: Timeline-based clip editing
- **Performance profiler**: Frame graph visualization

Currently in early planning stages. See module ROADMAP for milestones.

## Diagnostics Shell

Interactive shell for runtime diagnostics:

```bash
# Start diagnostics shell
python scripts/diagnostics/shell.py --runtime build/lib/libengine_runtime.so

# Interactive commands
> show metrics runtime.lifecycle.*
> show validation scene
> export snapshot.json
> reload assets/meshes/character.obj
```

The shell integrates with the hot reload infrastructure (`CC-002`) to trigger asset reloads from the command line.

## Testing

Tools tests validate:
- Telemetry viewer accuracy (`tests/test_telemetry_viewer.py`)
- ImGui integration (`tests/test_imgui.cpp`)
- Packaging script correctness (`tests/test_packaging.py`)
- Asset processor validation (`tests/test_asset_processor.py`)

Run tests:
```bash
pytest scripts/tests/tools/
ctest --preset clang-debug -R tools  # When enabled
```

## Dependencies

- **Core**: Telemetry schema, diagnostics data structures
- **Runtime**: Diagnostics API, C bindings for tooling
- **Python 3.12+**: For CLI tools and automation scripts
- **Dear ImGui**: Debug UI framework
- **All engine modules**: For comprehensive diagnostics coverage

## Related Documentation

- [`../../ROADMAP.md`](../../ROADMAP.md): Tools module status in roadmap
- [`../../design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md): Telemetry metric definitions
- [`../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md): How to add telemetry
- [`../runtime/DIAGNOSTICS.md`](../runtime/DIAGNOSTICS.md): Runtime diagnostics reference
- Python tooling: `python/README.md`, `scripts/README.md`

## Current Status

The tools module is undergoing modularization and is currently **disabled in the build**. The comment in the root `CMakeLists.txt` indicates:

```cmake
#tools # Disabled for now as tools are not modularized yet
```

Once modularization is complete, the module will be re-enabled and integrated into the regular build process. Track progress in the main README's module status table.

## Current State

- Editor/profiling/pipeline automation staging area with the telemetry viewer CLI surfacing runtime snapshots, Dear ImGui integration for diagnostics UI, and profiler utilities; runtime packaging script available for CI.

## Usage

- Python CLI tools live under `scripts/diagnostics/` and `scripts/tools/`.
- Run tools tests:
  - `pytest scripts/tests/`
  - `ctest --preset linux-gcc-debug -R tools` (when C++ tools are enabled)

## TODO / Next Steps

- Adopt the runtime packaging script in CI pipelines, monitor artefact manifests, and gather feedback for the next diagnostics viewer iteration (`TL-110`); see ../../ROADMAP.md
- Add Chrome trace export and document workflows for performance investigations (`TL-120`); see ../../ROADMAP.md
