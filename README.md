# Test Engine Workspace

## Agentic Workflow Snapshot

The workspace hosts a modular C++20 engine prototype. Each subsystem builds as an independent library under `engine/` and exports headers through `engine::headers`, while `docs/`, `python/`, and `scripts/` capture design notes, automation helpers, and build orchestration. Follow this breadcrumb trail at the start of every session:

1. Load [AGENTS.md](AGENTS.md) and follow the Workflow Blueprint (Sections 0.1–0.7) to build the shared context ladder, phase checklists, and quality instrumentation for the session.
2. Review [docs/NAVIGATION.md](docs/NAVIGATION.md) to confirm documentation precedence and locate module-level READMEs or design notes required by the context ladder.
3. Confirm the initiative or module you are touching in [docs/ROADMAP.md](docs/ROADMAP.md) and the relevant `docs/modules/<name>/` README/ROADMAP pair.
4. Open the matching task record under [hybrid_workflow/backlog/](hybrid_workflow/backlog/) to understand acceptance criteria, metadata, and gates before editing code.
5. Update documentation, task checklists, and roadmap status in the same change set so the agent + human workflow stays consistent with the deliverable matrix.

## Workspace Snapshot

| Module | Health | Current Capability | Next Step |
| --- | --- | --- | --- |
| Animation | ✅ Stable | Deterministic clip sampling, validation, JSON import/export, blend-tree controllers, structured error reporting, and linear blend skinning transform generation consumed by the runtime pose system. | Maintain backlog hygiene and support upcoming state-machine authoring guidance. |
| Assets | ✅ Stable | Generational handle caches for meshes, point clouds, graphs, textures, shaders, and materials with hot-reload callbacks driven by the filesystem watcher and async queue instrumentation publishing runtime telemetry for streaming diagnostics; diagnostics shell surfaces recent asset reload failures with actionable hints. Texture decoding now normalises colour space metadata, builds CPU mip chains, and supports HDR/LDR pipelines through stb_image-backed loaders. Streaming report exports hot-reload metrics for CI automation, emits a textual dashboard, and produces Chrome trace counter captures for dashboards, while OBJ reloads without vertices/faces are rejected to keep telemetry accurate. | Plan the next dataset refresh and keep ingestion automation deterministic as new manifests land. |
| Compute | ✅ Stable | Kernel dispatcher with per-kernel telemetry, runtime integration sample capturing queue assignments/jitter baselines, backend capability probing, dependency cycle analysis tooling, dispatcher extension guidance, and math helpers for identity transforms. | Review dispatcher follow-ups after AI-004 integration demos. |
| Core | ✅ Stable | EnTT-backed registry façade, subsystem discovery helpers, module bootstrap plumbing, and dependency cycle diagnostics protecting prior architecture decisions. | Keep runtime packaging automation validated in CI and document instrumentation updates. |
| Geometry | ✅ Stable | `SurfaceMesh` utilities, halfedge conversions, procedural primitives, ASCII IO, kd-tree/octree accelerators, CPU linear blend skinning deformers, surface topology analysis, spatial query telemetry, adaptive remeshing, UV reuse, ABF++ parameterisation, remeshing telemetry, and the `geometry_remesh` CLI for offline remeshing/UV generation with telemetry-aligned summaries. | Curate remeshing datasets that feed the AI-004 harness and case studies. |
| IO | ✅ Stable | Geometry/animation import-export wrappers, plugin-ready handlers, cache policy scaffolding, and curated fuzz corpus with regression coverage. | Coordinate libFuzzer CI enablement and expand telemetry instrumentation. |
| Math | ✅ Stable | Vector/matrix/quaternion primitives, orthonormal basis helpers, transform utilities, and analytic solvers for linear systems and low-degree polynomials. | Finish solver telemetry and documentation refresh alongside module backlog grooming. |
| Physics | ✅ Stable | Rigid-body world with mass clamping, damping, configurable sub-stepping, collider support, and sweep-and-prune broad phase plus collision telemetry. | Plan collision telemetry trend automation with runtime and diagnostics teams. |
| Platform | ✅ Stable | Virtual filesystem providers, filesystem watcher abstraction for hot reload, backend selection plumbing, and mocked window/input services pending OS integrations. | Track SDL parity and watcher guidance follow-ups; update developer docs as new OS targets enter planning. |
| Rendering | ✅ Stable | Command encoder integration (`T-0119`) and GPU resource providers (`T-0120`) now execute frame-graph passes against live OpenGL/Vulkan resources with retention controls, telemetry, and runtime presentation wiring. | Coordinate with the completed [`RT-410`](hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) adapters to stage TL-310 tooling bring-up. |
| Runtime | ✅ Stable | `RuntimeHost` now executes a declarative `RuntimeLoopPlan` with per-phase telemetry, deterministic stage planning, and shared presentation adapters across mock/OpenGL backends per [`ADR-0008`](docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md). | Focus on TL-310 follow-ups leveraging the archived [`RT-410`](hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) hooks. |
| Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | Keep diagnostics samples aligned with runtime validation policies and document findings. |
| Tools | 🔧 Feature-gated | Tools module compiles when `ENGINE_ENABLE_TOOLS=ON` (the default for repository presets), restoring Dear ImGui helpers, panel registry tests, and diagnostics automation while the editor harness work in [`TL-310`](hybrid_workflow/backlog/TL-310-editor-foundations.md) continues; `pytest scripts/tests/test_editor_smoke.py` now drives the compiled `test_tools_module` binary headlessly to exercise the panel registry, sandbox configuration loader, and runtime panel bridge. | Sequence TL-311–TL-314 diagnostic panels once TL-310 lands and document the expanded editor smoke coverage for PM-510 demos. |

### Directory Map

- **`docs/`** – Design records, module READMEs, task trackers, and reusable templates. Start here to align on priorities. The
  AI-004 prototyping playbook lives at
  [`docs/design/AI-004-prototyping-playbook.md`](docs/design/AI-004-prototyping-playbook.md) and captures the schema → dataset →
  harness workflow.
  Architecture diagrams are stored under `docs/architecture/`, while documentation templates (including the research paper template) live in `docs/templates/`.
- **`engine/`** – Native subsystems with headers, sources, and associated tests.
- **`python/`** – Runtime loaders and utilities that mirror C++ APIs; now ships
  a manually curated `.pyi` stub for `engine3g.loader` as part of the Python bindings refresh.
- **`scripts/`** – Build, validation, CI, and diagnostics orchestration entry points.
- **`third_party/`** – Vendored dependencies (EnTT, Dear ImGui, spdlog, GoogleTest, ...). Update only when dependency baselines shift.

### Platform Backend Selection

Backend selection combines a compile-time default with a runtime override while
respecting capability requirements declared in `WindowConfig`.

**Build-time default**

- Provide `-DENGINE_WINDOW_BACKEND=<GLFW|MOCK|AUTO>` during configuration.
- The cache value is normalised and embedded as `ENGINE_PLATFORM_DEFAULT_BACKEND`;
  `AUTO` disables the build-time preference.
- Presets under `scripts/build/presets/` pin sensible defaults (desktop = GLFW,
  CI/headless = MOCK).

**Runtime override**

- `ENGINE_PLATFORM_WINDOW_BACKEND` accepts `auto`, `glfw`, or `mock`
  (case-insensitive). Invalid values degrade to `mock` to keep automation
  deterministic.
- When the override targets a concrete backend, the selector still appends the
  mock backend as a fallback so tests continue operating if capability checks
  fail.

**Capability matrix**

| Backend | Headless Safe | Native Surface |
| --- | --- | --- |
| GLFW | ✅ | ✅ |
| Mock | ✅ | ❌ |

GLFW honours the headless capability requirement by creating hidden windows automatically when a configuration requests a
headless-safe backend, keeping CI automation deterministic.

`WindowConfig::CapabilityRequirements` filters out ineligible backends before a
selection is attempted. Direct requests for an incompatible backend return
clear error messages to surface missing capabilities. See
[`docs/modules/platform/README.md`](docs/modules/platform/README.md) for the full
backend selection reference.

**Automatic fallback order**

1. Runtime override (`ENGINE_PLATFORM_WINDOW_BACKEND`), then mock if required.
2. Build-time default when it maps to an available backend.
3. Remaining compiled backends in deterministic order (GLFW → Mock),
   guarded by `ENGINE_PLATFORM_HAS_*` macros.

Toggle GLFW fetching/building using `-DENGINE_ENABLE_GLFW=<ON|OFF>`. Missing X11
development headers automatically demote presets to the mock backend until
dependencies are restored.

The CMake configuration now disables `ENGINE_ENABLE_GLFW` automatically when the
GLFW target cannot be generated (for example, if the dependency fetch fails or
required headers are unavailable) so configure/build steps continue to succeed
with the mock backend.

## Execution Backlog Overview

The application-readiness plan is maintained in [docs/ROADMAP.md](docs/ROADMAP.md) with active tasks managed in [hybrid_workflow/backlog/](hybrid_workflow/backlog/). Current focus areas:

- **Phase 1 – Kickoff Ready (Completed):** [DC-040](docs/backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md), [DC-041](docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md), [RT-320](docs/backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md).
- **Phase 2 – Harness & Datasets (Completed):** [AS-330](docs/backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md), [TL-210](docs/backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md), [RT-321](docs/backlog/archive/RT_321_PROTOTYPING_CASE_STUDIES.md).
- **Phase 3 – Benchmark Confidence (Priority 4):** [CC-310](docs/backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md), [CC-311](docs/backlog/archive/CC_311_BENCHMARK_VISUALISATION.md).
- **Phase 4 – GPU Execution & Tooling (Priorities 1–2):** Joint GPU enablement milestone covering [T-0120 (complete)](hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md) + [T-0119](hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md), archived runtime presentation delivery in [RT-410](hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md), coordinated tooling reactivation in [TL-310](hybrid_workflow/backlog/TL-310-editor-foundations.md), and cross-module integration cadence in [PM-510](hybrid_workflow/backlog/PM-510-weekly-integration-demos.md).

See the roadmap for risk owners, success metrics, and archival history.


## Build & Test Workflow

### Prerequisites

- **Compilers** – C++20-capable toolchain. Validated with **Clang 22.0**, **GCC 13.2**, and **MSVC 19.38**; minimum supported versions remain **Clang ≥ 22**, **GCC ≥ 12**, **MSVC ≥ 19.34**.
- **Build system** – **CMake ≥ 3.20** (tested with 3.28.3) plus **Ninja ≥ 1.11** or the Visual Studio 2022 generator.
  - If Ninja isn't installed, use the Unix Makefiles presets added under `scripts/build/presets/linux.json` (see commands below).
- **Python** – Python 3.12+ with `pip` for scripts and harnesses.
- **Host libraries** – Platform SDKs for the rendering backend you target (Vulkan SDK 1.3.x, DirectX 12 Agility SDK, system OpenGL drivers). Linux builds enabling GLFW require `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev`.

### Configure and Build

```bash
cmake --preset linux-gcc-debug          # CPU-only (Ninja)
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug

# or configure the CUDA variant
cmake --preset linux-gcc-debug-cuda

# If Ninja isn't available, use the Makefiles variants
cmake --preset linux-gcc-debug-make
cmake --build --preset linux-gcc-debug-make
ctest --preset linux-gcc-debug-make
```

### Test & Validate

```bash
ctest --preset linux-gcc-debug                 # C++ unit/integration suites
pytest python/tests scripts/tests              # Python coverage
python scripts/validate_docs.py                # Cross-link validation
```

Record the exact commands and outcomes in PR descriptions to keep automation reproducible.
