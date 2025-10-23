# Test Engine Workspace

## Agentic Workflow Snapshot

The workspace hosts a modular C++20 engine prototype. Each subsystem builds as an independent library under `engine/` and exports headers through `engine::headers`, while `docs/`, `python/`, and `scripts/` capture design notes, automation helpers, and build orchestration. Follow this breadcrumb trail at the start of every session:

1. Review [docs/NAVIGATION.md](docs/NAVIGATION.md) for documentation navigation and task routing order.
2. Confirm the initiative or module you are touching in [docs/ROADMAP.md](docs/ROADMAP.md) and the relevant `docs/modules/<name>/` README/ROADMAP pair.
3. Open the matching record under [docs/tasks/](docs/tasks/) to understand acceptance criteria and status notes before editing code.
4. Update documentation, task checklists, and roadmap status together to keep the agent + human workflow in sync.

## Workspace Snapshot

| Module | Health | Current Capability | Next Task |
| --- | --- | --- | --- |
| Animation | ✅ Stable | Deterministic clip sampling, validation, JSON import/export, blend-tree controllers, structured error reporting, and linear blend skinning transform generation consumed by the runtime pose system. | `AN-230`: prototype GPU/parallel sampling benchmarks once compute queue extensions land. |
| Assets | ✅ Stable | Generational handle caches for meshes, point clouds, graphs, textures, shaders, and materials with hot-reload callbacks driven by the filesystem watcher and async queue instrumentation publishing runtime telemetry for streaming diagnostics; diagnostics shell surfaces recent asset reload failures with actionable hints. Streaming report now exports hot-reload metrics for CI automation, and OBJ reloads without vertices/faces are rejected to keep telemetry accurate. | `TL-120`: wire asset reload telemetry into diagnostics dashboards. |
| Compute | ✅ Stable | Kernel dispatcher with per-kernel telemetry, backend capability probing, dependency cycle analysis tooling, dispatcher extension guidance, and math helpers for identity transforms. | `CO-170`: prototype runtime integration sample showing dispatcher orchestration. |
| Core | ✅ Stable | EnTT-backed registry façade, subsystem discovery helpers, module bootstrap plumbing, and dependency cycle diagnostics protecting `DC-001`. | Keep the new runtime packaging automation wired into CI so initialization failure guidance remains validated and telemetry tooling stays reproducible. |
| Geometry | ✅ Stable | `SurfaceMesh` utilities, halfedge conversions, procedural primitives, ASCII IO, kd-tree/octree accelerators, CPU linear blend skinning deformers, and spatial query telemetry aligned with the diagnostics schema. | Sequence remeshing execution milestones derived from the published RFP (`GE-221+` planning). |
| IO | ✅ Stable | Geometry/animation import-export wrappers, plugin-ready handlers, cache policy scaffolding, and curated fuzz corpus with regression coverage. | Coordinate CI enablement for the libFuzzer harness and extend `IO-240` telemetry instrumentation. |
| Math | ✅ Stable | Vector/matrix/quaternion primitives, orthonormal basis helpers, transform utilities, and analytic solvers for linear systems and low-degree polynomials. | External-format conversion cheatsheet published in `docs/modules/math/format_conversions.md`; `MA-130` scopes conversion telemetry metrics and solver telemetry follow-up. |
| Physics | ✅ Stable | Rigid-body world with mass clamping, damping, configurable sub-stepping, collider support, and sweep-and-prune broad phase plus collision telemetry. | Scope automation for long-term collision telemetry trends (post-`PH-430`). |
| Platform | ✅ Stable | Virtual filesystem providers, filesystem watcher abstraction for hot reload, backend selection plumbing, and mocked window/input services pending OS integrations. | Scope SDL backend implementation work using the new parity checklist (`PL-215`) to advance `DC-003`. |
| Rendering | ✅ Stable | Frame-graph compilation/execution, command encoder hooks, resource lifetime tracking, Vulkan scheduler prototype, and backend validation metrics covering all providers. | Monitor backend parity telemetry and scope future backend feature coverage. |
| Runtime | ✅ Stable | `RuntimeHost` orchestration advancing animation, compute-driven physics, CPU linear blend skinning, geometry deformation, asynchronous asset requests, and submission into the rendering pipeline. | `AI-002`: extend async streaming diagnostics once assets hot-reload callbacks land. |
| Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | Keep the `SC-225` hierarchy diagnostics samples and dashboards aligned with the `runtime.scene_validation.alert_level` policy introduced by `SC-230`. |
| Tools | ✅ Stable | Editor/profiling/pipeline automation staging area with the telemetry viewer CLI surfacing runtime snapshots, Dear ImGui integration for diagnostics UI, and profiler utilities for performance analysis. | Adopt the runtime packaging script in CI pipelines, monitor artefact manifests, and gather feedback for the next diagnostics viewer iteration. |

### Directory Map

- **`docs/`** – Design records, module READMEs, task trackers, and reusable templates. Start here to align on priorities.
- **`engine/`** – Native subsystems with headers, sources, and associated tests.
- **`python/`** – Runtime loaders and utilities that mirror C++ APIs; now ships
  a manually curated `.pyi` stub for `engine3g.loader` as part of `PY-001`.
- **`scripts/`** – Build, validation, CI, and diagnostics orchestration entry points.
- **`third_party/`** – Vendored dependencies (EnTT, Dear ImGui, spdlog, GoogleTest, ...). Update only when dependency baselines shift.

### Platform Backend Selection

Backend selection combines a compile-time default with a runtime override while
respecting capability requirements declared in `WindowConfig`.

**Build-time default**

- Provide `-DENGINE_WINDOW_BACKEND=<GLFW|SDL|MOCK|AUTO>` during configuration.
- The cache value is normalised and embedded as `ENGINE_PLATFORM_DEFAULT_BACKEND`;
  `AUTO` disables the build-time preference.
- Presets under `scripts/build/presets/` pin sensible defaults (desktop = GLFW,
  CI/headless = MOCK).

**Runtime override**

- `ENGINE_PLATFORM_WINDOW_BACKEND` accepts `auto`, `glfw`, `sdl`, or `mock`
  (case-insensitive). Invalid values degrade to `mock` to keep automation
  deterministic.
- When the override targets a concrete backend, the selector still appends the
  mock backend as a fallback so tests continue operating if capability checks
  fail.

**Capability matrix**

| Backend | Headless Safe | Native Surface |
| --- | --- | --- |
| GLFW | ❌ | ✅ |
| SDL | ✅ | ✅ |
| Mock | ✅ | ❌ |

`WindowConfig::CapabilityRequirements` filters out ineligible backends before a
selection is attempted. Direct requests for an incompatible backend return
clear error messages to surface missing capabilities. See
[`docs/modules/platform/README.md`](docs/modules/platform/README.md) for the full
backend selection reference.

**Automatic fallback order**

1. Runtime override (`ENGINE_PLATFORM_WINDOW_BACKEND`), then mock if required.
2. Build-time default when it maps to an available backend.
3. Remaining compiled backends in deterministic order (GLFW → SDL → Mock),
   guarded by `ENGINE_PLATFORM_HAS_*` macros.

Toggle GLFW fetching/building using `-DENGINE_ENABLE_GLFW=<ON|OFF>`. Missing X11
development headers automatically demote presets to the mock backend until
dependencies are restored.

The CMake configuration now disables `ENGINE_ENABLE_GLFW` automatically when the
GLFW target cannot be generated (for example, if the dependency fetch fails or
required headers are unavailable) so configure/build steps continue to succeed
with the mock backend.

## Execution Backlog Overview

The architecture improvement plan is the authoritative backlog. The summary below mirrors [docs/ROADMAP.md](docs/ROADMAP.md) and highlights the active slices agents should track.

### Cross-Cutting Initiatives

| ID | Intent | Current Focus | Owner(s) | Status |
| --- | --- | --- | --- | --- |
| `DC-004` | Standardise error handling across modules. | Migrate IO to `Result<T>` and publish migration guide. | Core, IO | ✅ Done |
| `AI-001` | Handle-based lifetime management across assets + rendering. | Debug validation hooks and telemetry wired across caches and rendering entry points. | Assets, Rendering | ✅ Done |
| `AI-002` | Async asset streaming with telemetry and runtime integration. | Land async queue instrumentation and runtime metrics bridge. | Assets, Runtime | 🔄 In Progress |
| `AI-003` | Frame-graph metadata + queue affinity for backend parity. | Publish metadata schema and align runtime submission invariants. | Rendering, Runtime | ✅ Done |
| `RT-002` | Persistent physics manifolds with benchmarking. | ✅ Completed – manifold cache, telemetry, and collision benchmark harness captured in CI. | Physics | ✅ Done |
| `RT-003` | Vulkan backend parity and documentation. | Align runtime submission surfaces and publish backend checklist. | Rendering, Runtime | ✅ Done |
| `RT-005` | Scene hierarchy validation + diagnostics. | Integrate cycle detection and reporting hooks. | Scene, Runtime | ✅ Done |
| `RT-006` | IO signature hardening + fuzzing. | Wire signature database and libFuzzer corpus seeding. | IO | 🟠 Blocked on fuzz harness infra |
| `CC-001` | Telemetry instrumentation and viewer. | Telemetry schema, viewer CLI, and instrumentation guide published. | Core, Tools | ✅ Done |
| `CC-002` | Hot reload infrastructure. | Hot reload callbacks publish telemetry and diagnostics viewer surfaces reload failures. | Assets, Platform | ✅ Done |

### Sprint Horizon

| Horizon | Rank | Initiative | Primary Deliverable |
| --- | --- | --- | --- |
| Sprint 1 | 1 | `AI-003` | ✅ Completed – runtime diagnostics capture frame-graph metadata for Vulkan parity validation. |
|         | 2 | `RT-003` | ✅ Completed – integration regression locks Vulkan submission determinism (`RT-003.3`). |
|         | 3 | `TI-001` | Integration harness smoke suite stabilised post `T-0118`. |
| Sprint 2–3 | 1 | `DC-004` | IO migration merged with documentation updates. |
|             | 2 | `AI-001` | ✅ Completed – handle validation hooks emit telemetry in debug builds. |
|             | 3 | `RT-002` | ✅ Completed – collision benchmark harness records throughput metrics for CI. |
|             | 4 | `DI-001` | Module README refresh complete (follow template below). |
| Mid-Term (M4–M5) | 1 | `AI-002` | Async streaming MVP validated end-to-end. |
|                   | 2 | `CC-001` | Diagnostics viewer prototype available. |
|                   | 3 | `PY-001` | Core bindings and `.pyi` stubs published. |

Reconcile this table with the roadmap whenever priorities change. Update both documents in the same change to prevent drift.

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
