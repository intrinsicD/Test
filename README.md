# Test Engine Workspace

## Agentic Workflow Snapshot

The workspace hosts a modular C++20 engine prototype. Each subsystem builds as an independent library under `engine/` and exports headers through `engine::headers`, while `docs/`, `python/`, and `scripts/` capture design notes, automation helpers, and build orchestration. Follow this breadcrumb trail at the start of every session:

1. Load [AGENTS.md](AGENTS.md) and follow the Workflow Blueprint (Sections 0.1–0.7) to build the shared context ladder, phase checklists, and quality instrumentation for the session.
2. Review [docs/NAVIGATION.md](docs/NAVIGATION.md) to confirm documentation precedence and locate module-level READMEs or design notes required by the context ladder.
3. Confirm the initiative or module you are touching in [docs/ROADMAP.md](docs/ROADMAP.md) and the relevant `docs/modules/<name>/` README/ROADMAP pair.
4. Open the matching record under [docs/backlog/](docs/backlog/) to understand acceptance criteria, role roster commitments, and required artefacts before editing code.
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
| Rendering | ⚠️ Blocked | Frame-graph infrastructure exists, but GPU resource provider and command encoder work never shipped, leaving OpenGL/Vulkan backends limited to recording providers with no real shader or buffer execution. | Execute the joint GPU enablement milestone pairing [`T-0120`](docs/backlog/active/T-0120-gpu-resource-provider.md) and [`T-0119`](docs/backlog/active/T-0119-command-encoder-integration.md) with shared design reviews and weekly backend demos to restore real GPU execution. |
| Runtime | ⚠️ At Risk | `RuntimeHost` now executes a declarative `RuntimeLoopPlan` with per-phase telemetry and a presentation dispatch hook, yet ADR-0008's presentation backends and synchronisation APIs remain outstanding for GPU submission and tooling reuse. | Start [`RT-410`](docs/backlog/active/RT-410-runtime-stage-planner.md) in parallel with the GPU milestone so presentation adapters and synchronisation land ahead of backend demos. |
| Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | Keep diagnostics samples aligned with runtime validation policies and document findings. |
| Tools | 🚧 Disabled | Editor/tooling module remains disabled in the build, but the shared panel registry now centralises Dear ImGui diagnostics with unit coverage while editor harness integration remains outstanding. | Sequence [`TL-310`](docs/backlog/active/TL-310-editor-foundations.md) alongside the stage planner work to re-enable builds and hook diagnostics into the shared demos. |

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

The application-readiness plan is maintained in [docs/ROADMAP.md](docs/ROADMAP.md) with detailed backlog entries under [docs/backlog/](docs/backlog/). Current focus areas:

- **Phase 1 – Kickoff Ready (Completed):** [DC-040](docs/backlog/archive/DC-040-ai-004-configuration-schema.md), [DC-041](docs/backlog/archive/DC-041-ai-004-kickoff-readiness.md), [RT-320](docs/backlog/archive/RT-320-runtime-prototyping-harness.md).
- **Phase 2 – Harness & Datasets (Completed):** [AS-330](docs/backlog/archive/AS-330-reference-dataset-packages.md), [TL-210](docs/backlog/archive/TL-210-experiment-sandbox-ui.md), [RT-321](docs/backlog/archive/RT-321-prototyping-case-studies.md).
- **Phase 3 – Benchmark Confidence (Priority 4):** [CC-310](docs/backlog/archive/CC-310-comparative-benchmark-automation.md), [CC-311](docs/backlog/archive/CC-311-benchmark-visualisation.md).
- **Phase 4 – GPU Execution & Tooling (Priorities 1–2):** Joint GPU enablement milestone covering [T-0120](docs/backlog/active/T-0120-gpu-resource-provider.md) + [T-0119](docs/backlog/active/T-0119-command-encoder-integration.md), parallel runtime presentation work in [RT-410](docs/backlog/active/RT-410-runtime-stage-planner.md), coordinated tooling reactivation in [TL-310](docs/backlog/active/TL-310-editor-foundations.md), and cross-module integration cadence in [PM-510](docs/backlog/active/PM-510-weekly-integration-demos.md).

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
