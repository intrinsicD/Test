# Test Engine Workspace

## Agentic Workflow Snapshot

The workspace hosts a modular C++20 engine prototype. Each subsystem builds as an independent library under `engine/` and exports headers through `engine::headers`, while `docs/`, `python/`, and `scripts/` capture design notes, automation helpers, and build orchestration. Follow this breadcrumb trail at the start of every session:

1. Review [docs/README.md](docs/README.md) for the current working agreement and task routing order.
2. Confirm the initiative or module you are touching in [docs/ROADMAP.md](docs/ROADMAP.md) and the relevant `docs/modules/<name>/` README/ROADMAP pair.
3. Open the matching record under [docs/tasks/](docs/tasks/) to understand acceptance criteria and status notes before editing code.
4. Update documentation, task checklists, and roadmap status together to keep the agent + human workflow in sync.

## Workspace Snapshot

| Module | Health | Current Capability | Next Task |
| --- | --- | --- | --- |
| Animation | ✅ Stable | Deterministic clip sampling, validation, JSON import/export, blend-tree controllers, structured error reporting, and linear blend skinning transform generation consumed by the runtime pose system. | `AN-230`: prototype GPU/parallel sampling benchmarks once compute queue extensions land. |
| Assets | 🔄 In Progress | Generational handle caches for meshes, point clouds, graphs, textures, shaders, and materials with hot-reload callbacks driven by the filesystem watcher and async queue instrumentation publishing runtime telemetry for streaming diagnostics. | `AS-330`: diagnostics shell reload surfacing. |
| Compute | ✅ Stable | Kernel dispatcher with per-kernel telemetry, backend capability probing, dispatcher extension guidance, and math helpers for identity transforms. | `CO-150`: implement kernel dependency cycle detection tooling. |
| Core | ✅ Stable | EnTT-backed registry façade, subsystem discovery helpers, and module bootstrap plumbing consumed by higher-level systems. | `CR-135`: subsystem dependency diagnostics to harden `DC-001`. |
| Geometry | ✅ Stable | `SurfaceMesh` utilities, halfedge conversions, procedural primitives, ASCII IO, kd-tree/octree accelerators, and CPU linear blend skinning deformers. | `GE-205`: benchmark accelerated normal recomputation for `TI-002`. |
| IO | 🔄 In Progress | Geometry/animation import-export wrappers, plugin-ready handlers, and cache policy scaffolding. | `IO-221`: integrate signature database + fuzz harness towards `RT-006`. |
| Math | ✅ Stable | Vector/matrix/quaternion primitives, orthonormal basis helpers, and transform utilities feeding animation, geometry, and physics. | `MA-110`: add SIMD validation targets aligned with `TI-003`. |
| Physics | ✅ Stable | Rigid-body world with mass clamping, damping, configurable sub-stepping, collider support, and sweep-and-prune broad phase plus collision telemetry. | Scope automation for long-term collision telemetry trends (post-`PH-430`). |
| Platform | ✅ Stable | Virtual filesystem providers, filesystem watcher abstraction for hot reload, backend selection plumbing, and mocked window/input services pending OS integrations. | `PL-215`: publish SDL backend parity checklist for `DC-003`. |
| Rendering | 🔄 In Progress | Frame-graph compilation/execution, command encoder hooks, resource lifetime tracking, and Vulkan scheduler prototype. | `RE-530`: backend validation tooling and parity tracking follow-up. |
| Runtime | ✅ Stable | `RuntimeHost` orchestration advancing animation, compute-driven physics, CPU linear blend skinning, geometry deformation, and submission into the rendering pipeline. | `AI-002`: extend async streaming diagnostics once assets hot-reload callbacks land. |
| Scene | ✅ Stable | Entity façade, hierarchy and transform propagation, deterministic serialisation, and component helpers. | `SC-225`: publish hierarchy diagnostics samples and alerting guidance follow-up. |
| Tools | 🔄 In Progress | Editor/profiling/pipeline automation staging area with the telemetry viewer CLI surfacing runtime snapshots. | `TL-110`: document tooling invocation and troubleshooting. |

### Directory Map

- **`docs/`** – Design records, module READMEs, task trackers, and reusable templates. Start here to align on priorities.
- **`engine/`** – Native subsystems with headers, sources, and associated tests.
- **`python/`** – Runtime loaders and utilities that mirror C++ APIs.
- **`scripts/`** – Build, validation, CI, and diagnostics orchestration entry points.
- **`third_party/`** – Vendored dependencies (EnTT, Dear ImGui, spdlog, GoogleTest, ...). Update only when dependency baselines shift.

### Platform Backend Selection

- Configure the default window backend with `-DENGINE_WINDOW_BACKEND=<GLFW|SDL|MOCK>` during CMake configuration. Presets default to `GLFW`; headless CI jobs force `MOCK`.
- Override backends at runtime via `ENGINE_PLATFORM_WINDOW_BACKEND` (`auto`, `mock`, `glfw`, `sdl`). Automatic selection honours `WindowConfig::capability_requirements`, falling back when a backend cannot satisfy surface/headless constraints.
- Toggle GLFW fetching/building using `-DENGINE_ENABLE_GLFW=<ON|OFF>`. Missing X11 development headers automatically demote presets to the mock backend until dependencies are restored.

## Execution Backlog Overview

The architecture improvement plan is the authoritative backlog. The summary below mirrors [docs/ROADMAP.md](docs/ROADMAP.md#architecture-improvement-plan) and highlights the active slices agents should track.

### Cross-Cutting Initiatives

| ID | Intent | Current Focus | Owner(s) | Status |
| --- | --- | --- | --- | --- |
| `DC-004` | Standardise error handling across modules. | Migrate IO to `Result<T>` and publish migration guide. | Core, IO | ✅ Done |
| `AI-001` | Handle-based lifetime management across assets + rendering. | Extend debug validation hooks and document ownership patterns. | Assets, Rendering | 🔄 In Progress |
| `AI-002` | Async asset streaming with telemetry and runtime integration. | Land async queue instrumentation and runtime metrics bridge. | Assets, Runtime | 🟡 Blocked on `AI-001` docs refresh |
| `AI-003` | Frame-graph metadata + queue affinity for backend parity. | Publish metadata schema and align runtime submission invariants. | Rendering, Runtime | ✅ Done |
| `RT-002` | Persistent physics manifolds with benchmarking. | ✅ Completed – manifold cache, telemetry, and collision benchmark harness captured in CI. | Physics | ✅ Done |
| `RT-003` | Vulkan backend parity and documentation. | Align runtime submission surfaces and publish backend checklist. | Rendering, Runtime | 🔄 In Progress |
| `RT-005` | Scene hierarchy validation + diagnostics. | Integrate cycle detection and reporting hooks. | Scene, Runtime | ✅ Done |
| `RT-006` | IO signature hardening + fuzzing. | Wire signature database and libFuzzer corpus seeding. | IO | 🟠 Blocked on fuzz harness infra |
| `CC-001` | Telemetry instrumentation and viewer. | Telemetry schema, viewer CLI, and instrumentation guide published. | Core, Tools | ✅ Done |
| `CC-002` | Hot reload infrastructure. | Filesystem watcher landed; integrate cache callbacks and diagnostics telemetry next. | Assets, Platform | 🔄 In Progress |

### Sprint Horizon

| Horizon | Rank | Initiative | Primary Deliverable |
| --- | --- | --- | --- |
| Sprint 1 | 1 | `AI-003` | ✅ Completed – runtime diagnostics capture frame-graph metadata for Vulkan parity validation. |
|         | 2 | `RT-003` | ✅ Completed – integration regression locks Vulkan submission determinism (`RT-003.3`). |
|         | 3 | `TI-001` | Integration harness smoke suite stabilised post `T-0118`. |
| Sprint 2–3 | 1 | `DC-004` | IO migration merged with documentation updates. |
|             | 2 | `AI-001` | Handle validation hooks turned on in debug builds. |
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
- **Python** – Python 3.12+ with `pip` for scripts and harnesses.
- **Host libraries** – Platform SDKs for the rendering backend you target (Vulkan SDK 1.3.x, DirectX 12 Agility SDK, system OpenGL drivers). Linux builds enabling GLFW require `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev`.

### Configure and Build

```bash
cmake --preset linux-gcc-debug          # CPU-only
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug

# or configure the CUDA variant
cmake --preset linux-gcc-debug-cuda
```

### Test & Validate

```bash
ctest --preset linux-gcc-debug                 # C++ unit/integration suites
pytest python/tests scripts/tests              # Python coverage
python scripts/validate_docs.py                # Cross-link validation
```

Record the exact commands and outcomes in PR descriptions to keep automation reproducible.
