# Test Engine Workspace

## Current State

The workspace hosts a modular C++20 engine prototype. Each subsystem builds as an
independent library under `engine/` and exports headers through
`engine::headers`, while `docs/`, `python/`, and `scripts/` capture design
notes, automation helpers, and build orchestration. The table below summarises
what is implemented today.

| Module | Implementation snapshot |
| --- | --- |
| Animation | Deterministic clip sampling, validation, JSON import/export, linear controllers, and parameter binding blend trees that the runtime consumes for pose evaluation. |
| Assets | Runtime caches for meshes, point clouds, graphs, textures, and shaders with hot-reload polling plus handle-based lookups; materials currently store descriptor bindings without file-backed reload. |
| Compute | A topological kernel dispatcher that measures execution time per kernel, reports backend availability, and exposes a lightweight math helper for identity transforms. |
| Core | EnTT-backed registry façade, entity helpers, and module discovery utilities that higher-level systems consume. |
| Geometry | `SurfaceMesh` utilities (normals, bounds, centroid), conversions to/from the halfedge core, procedural primitives, and ASCII import/export for meshes, graphs, and point clouds backed by spatial indices such as kd-trees and octrees. |
| IO | Geometry and animation import/export wrappers with format detection, a plugin-ready registry for mesh/point-cloud/graph handlers, and scaffolding for cache policies. |
| Math | Vector, matrix, quaternion, and transform primitives plus orthonormal basis helpers that feed animation, geometry, and physics. |
| Physics | Rigid-body world with mass clamping, damping, configurable substepping, sphere/AABB/capsule colliders, and sweep-and-prune broad-phase collision detection. |
| Platform | Virtual filesystem providers, backend selection plumbing, and mocked window/input services pending concrete OS integrations. |
| Rendering | Frame-graph compilation/execution, command encoder hooks, resource lifetime tracking, and a Vulkan-backed GPU scheduler prototype validating the submission interface. |
| Runtime | `RuntimeHost` orchestration that advances animation, drives physics via the compute dispatcher, deforms geometry, updates bounds, mirrors joint transforms into an EnTT-backed scene, and submits the scene to the forward pipeline through the GPU scheduler integration. |
| Scene | Entity façade with hierarchy and transform propagation systems, deterministic serialization/deserialization, and component helpers used by the runtime. |
| Tools | Staging area for editor, profiling, and pipeline automation features with roadmap-driven scaffolding. |

### Module Overview

- **`docs/`** – Centralised design records, API notes, and the reusable README template.
- **`engine/`** – Native subsystems organised by responsibility with matching tests and headers.
- **`python/`** – Loader and helper code that interacts with the compiled engine modules.
- **`scripts/`** – Build, validation, and CI orchestration entry points.
- **`third_party/`** – Vendored dependencies such as EnTT, Dear ImGui, spdlog, and GoogleTest.

### Platform Backend Selection

- Configure the default window backend using `-DENGINE_WINDOW_BACKEND=<GLFW|SDL|MOCK>` when generating builds. Presets default to `GLFW`, while headless CI jobs can explicitly set `MOCK`.
- Override the backend at runtime with `ENGINE_PLATFORM_WINDOW_BACKEND` (`auto`, `mock`, `glfw`, `sdl`). Automatic selection honours `WindowConfig::capability_requirements`, skipping backends that cannot run headless or provide a native surface when requested.
- Control whether GLFW is fetched and built with `-DENGINE_ENABLE_GLFW=<ON|OFF>`. When the required X11 development headers (for example `libxrandr-dev`) are missing the configure step automatically disables GLFW support and falls back to the mock backend until the dependency is available.

## Design Documentation Workflow

- **Start every session with [`docs/README.md`](docs/README.md).** It stitches together the working agreement in
  [`AGENTS.md`](AGENTS.md), subsystem invariants, and the task/specification records so AI assistants and human contributors
  follow the same breadcrumbs.
- The `docs/` tree captures architecture explorations, design rationales, and decision records that anchor large subsystem work.
  Capture design proposals here before landing major subsystems and reference the relevant records in pull requests so reviewers
  can trace intent.
- The cross-module architectural backlog is curated in the
  [central roadmap](docs/ROADMAP.md#architecture-improvement-plan). Review and update this plan when reprioritising milestones or
  when new technical debt emerges. The plan is partitioned into **Critical Design Corrections (DC)**, **Architecture Improvements
  (AI)**, **Roadmap TODOs (RT)**, **Documentation Improvements (DI)**, **Build System Enhancements (BS)**, **Testing Infrastructure
  (TI)**, **Python Bindings (PY)**, and **Cross-Cutting initiatives (CC)**.
- Keep README snapshots aligned with that plan so contributors can see which areas are currently emphasised without opening the
  full design record.
- As milestones conclude, update the associated notes and mirror outcomes into the roadmap to keep backlog discussions
  synchronised with implementation reality.
- Rendering and runtime coordination currently centres on the **Rendering/Runtime vertical slice**. The active work streams are
  codified as `AI-003` (frame-graph metadata alignment) and `RT-003` (Vulkan scheduler integration) in the architecture
  improvement plan. Their shared goal is a deterministic path from runtime orchestration into the rendering frame graph, realised
  through enriched resource descriptors, a backend-neutral scheduler, and runtime submission hooks. Capture the rationale and
  status in the plan so rendering and runtime teams stay aligned while animation, physics, and geometry continue acting as data
  providers. As deliverables land, record acceptance outcomes and link test coverage so future contributors understand why the
  slice was scoped this way and which regressions it guards against.

## Architecture Improvement Plan Snapshot

The architecture improvement plan is the canonical backlog for high-priority system work. The bullet points below summarise the
current items so this README remains an at-a-glance companion to the full
[roadmap entry](docs/ROADMAP.md#architecture-improvement-plan).

### Critical Design Corrections

- `DC-001` (**High**): Introduce subsystem interfaces, dependency injection, and plugin discovery in the runtime module.
- `DC-002` (**High**, depends on `DC-001`): Make CUDA optional with feature flags, dispatcher abstractions, and build preset
  updates.
- `DC-003` (**Medium**): Support GLFW, SDL, and mock window backends through a configurable factory and document backend
  selection.
- `DC-004` (**Medium**): Standardise error handling with `Result<T, Error>` types, error hierarchies, and IO pilot migration.

### Architecture Improvements

- `AI-001` (**High**, depends on `DC-004`): Roll out handle-based resource lifetime management across core, assets, and
  rendering.
- `AI-002` (**Medium**, depends on `AI-001` and `DC-001`): Build async asset streaming with background threading, futures, and
  telemetry.
- `AI-003` (**High**): Expand frame-graph metadata, queue affinity, and validation to unblock backend work.
- `AI-004` (**Medium**, depends on `DC-002`): Validate compute kernel dependencies with cycle detection and improved errors.

### Roadmap Advancement TODOs

- `RT-001` (**High**): Deliver the animation deformation pipeline with rig binding, LBS, and integration tests.
- `RT-002` (**High**): Implement persistent physics contact manifolds and benchmarking.
- `RT-003` (**High**, depends on `AI-003`): Prototype the Vulkan rendering backend and document prerequisites.
- `RT-004` (**High**): Add runtime lifecycle diagnostics, telemetry, and dashboards.
- `RT-005` (**High**): Validate scene hierarchies, guard against cycles, and expose validation reports.
- `RT-006` (**High**): Harden IO format detection with signature databases, fuzzing, and structured errors.

### Supporting Initiatives

- **Documentation (`DI-001`–`DI-003`)**: Standardise module READMEs, automate completeness checks, generate API references, and
  author ADRs.
- **Build System (`BS-001`–`BS-003`)**: Expand presets, formalise versioning, and clean CMake targets with accompanying docs.
- **Testing (`TI-001`–`TI-003`)**: Establish integration suites, performance benchmarks, and fuzzing harnesses with CI hooks.
- **Python (`PY-001`)**: Bootstrap core bindings, dependency management, and pytest coverage.
- **Cross-Cutting (`CC-001`, `CC-002`)**: Develop telemetry instrumentation and hot reload infrastructure across subsystems.
- **Milestone Coordination (`MC-001`, `MC-002`)**: Maintain milestone dashboards and synchronise module roadmaps through
  automation.

### Outstanding Backlog Focus

The following items remain open and should be prioritised when planning new work:

- **Compute dependency validation (`AI-004`)** – introduce explicit dependency metadata, perform cycle detection, and surface
  diagnostics for the dispatcher.
- **Physics constraint solver integration (`RT-002-FU1`)** – leverage the manifold callbacks to implement impulse resolution,
  extend telemetry with solver iterations, and document authoring workflows.
- **Scene hierarchy validation (`RT-005`)** – implement cycle and transform integrity checks, add reporting hooks, and document
  validation flows.
- **IO format detection hardening (`RT-006`)** – expand signature databases and integrate fuzzing harnesses beyond the structured
  errors that already landed.
- **Telemetry framework (`CC-001`)** – build a cross-module telemetry API, implement sinks, instrument hot paths, and ship a
  profiling viewer.
- **Hot reload infrastructure (`CC-002`)** – add filesystem watching, asset cache callbacks, transaction logging, integration
  tests, and documentation.
- **Benchmarking & fuzzing harnesses (`TI-002`, `TI-003`)** – land performance benchmarks, CI regression detection, IO fuzzing
  targets, corpora, and documentation.
- **Python bindings (`PY-001`)** – document binding generation, add dependencies, expose animation/geometry APIs, provide stubs,
  and add pytest coverage.
- **Documentation improvements (`DI-001`–`DI-003`)** – standardise READMEs, automate completeness checks, generate API
  references, and establish ADR infrastructure.
- **Build system hygiene (`BS-001`–`BS-003`)** – expand presets, document the versioning policy, and audit target dependency
  hygiene.
- **Milestone coordination (`MC-001`, `MC-002`)** – create milestone dashboards and keep module roadmaps synchronised with
  central planning.

### Priority Horizon

The plan ranks near-term execution focus as follows. Track status to ensure priorities remain synchronised with the detailed
roadmap.

| Horizon | Rank | Item | Status |
|---------|------|------|--------|
| Sprint 1 Focus | 1 | `AI-003` | In Progress — keep frame-graph metadata and runtime submission paths aligned across modules. |
|                 | 2 | `RT-001` | Complete — monitor regressions while extending deformation coverage. |
|                 | 3 | `TI-001` | Complete — integration suites remain the regression baseline. |
| Sprint 2-3 | 1 | `DC-004` | In Progress — finalise error-handling rollout and supporting documentation. |
|            | 2 | `AI-001` | In Progress — expand handle-based lifetime management across caches and rendering. |
|            | 3 | `RT-002` | Planned — ship persistent manifolds with constraint solving and telemetry. |
|            | 4 | `RT-004` | Complete — runtime lifecycle diagnostics surface through `RuntimeHost::diagnostics()` and the telemetry tooling. |
|            | 5 | `DI-001` | Planned — standardise module READMEs and automate completeness checks. |
| Mid-Term (M4-M5) | 1 | `AI-002` | Planned — stand up async asset streaming with telemetry integration. |
|                   | 2 | `RT-003` | In Progress — maintain parity between runtime submission and the Vulkan backend. |
|                   | 3 | `CC-001` | Planned — design the cross-module telemetry framework and viewer. |
|                   | 4 | `PY-001` | Planned — expose core bindings with pytest coverage. |

Revisit the improvement plan at the start of each milestone to adjust priorities, confirm dependencies, and align with evolving
product goals. Update both the plan and this snapshot together to prevent drift.

## Usage

### Prerequisites

- **Compilers** – C++20-capable toolchain. We validate changes with **Clang 22.0**, **GCC 13.2**, and **MSVC 19.38**; earlier revisions must still satisfy the minimum of **Clang ≥ 22**, **GCC ≥ 12**, or **MSVC ≥ 19.34** to compile the modules. Confirm availability with `clang --version`, `g++ --version`, or `cl.exe /?` before configuring.
- **Build system** – **CMake ≥ 3.20** (tested with 3.28.3) plus **Ninja ≥ 1.11** or the Visual Studio 2022 generator. Verify with `cmake --version` and `ninja --version` to ensure presets pick up the expected tools.
- **Python** – Python 3.12+ with `pip` for scripts and test harnesses.
- **Host libraries** – Platform SDKs/drivers for the rendering backends you plan to target (Vulkan SDK 1.3.x, DirectX 12 Agility SDK, or system OpenGL drivers). Linux builds that enable GLFW require `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev`.

### Configure and Build

```bash
cmake --preset linux-gcc-debug          # CPU-only
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug

# or configure the CUDA variant
cmake --preset linux-gcc-debug-cuda
cmake --build --preset linux-gcc-debug-cuda
ctest --preset linux-gcc-debug-cuda
```

Presets live under `scripts/build/` and currently cover Linux (GCC) and Windows (MSVC) compiler stacks. Additional variants can be invoked with `cmake --preset <name>` or orchestrated collectively via `scripts/ci/run_presets.py`. Each subsystem still produces a library named `engine_<subsystem>`; linking to any of them automatically imports the shared usage requirements published by `engine::project_options` and the aggregated headers exposed through `engine::headers`.

Subsystem availability can be tailored at configure time through the `ENGINE_ENABLE_<MODULE>` options (for example `-DENGINE_ENABLE_RENDERING=OFF`). Disabled modules are omitted from the default runtime subsystem registry but can be re-enabled explicitly by calling the helper configuration APIs in `engine/runtime/api.hpp`.

CUDA-oriented components stay disabled unless you opt into the dedicated CUDA presets (for example `linux-gcc-debug-cuda` or `windows-msvc-release-cuda`) or explicitly configure with `-DENGINE_ENABLE_CUDA=ON`. The presets set both `ENGINE_ENABLE_CUDA` and the matching subsystem toggle `ENGINE_ENABLE_COMPUTE_CUDA`, ensuring the optional `engine_compute_cuda` target is generated and integrated when desired while leaving CPU-only variants untouched by default.

### Testing

- **C++ suites** – Execute `ctest --preset <preset>` (for example `ctest --preset linux-gcc-debug`) to honour the generator and cache variables baked into the presets.
- **Documentation checks** – `python scripts/validate_docs.py` validates Markdown cross-references.

### Python Tooling

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt  # to be populated alongside tooling work
pytest
```

Ensure `ENGINE3G_LIBRARY_PATH` points to the directory containing the built shared libraries before invoking the loader.

### Maintenance Guidelines

- Adopt `docs/README_TEMPLATE.md` when adding or updating module READMEs.
- Keep module TODO bullets synchronised with the aggregate table below.
- Run the available tests and documentation checks before submitting changes.

## Architecture Overview

### Module Boundaries

Each functional domain (math, geometry, scene, rendering, physics, etc.) resides in its own top-level
subdirectory under [`engine/`](engine). Modules publish a minimal ABI surface through
`include/engine/<name>` headers, while the corresponding `src/` directories contain the shared-library
entry points. Shared infrastructure is imported explicitly; no module relies on transitive include paths.
Math sits at the base of the dependency graph, providing numerics for geometry, physics, and rendering.
Geometry builds on math to supply shapes, spatial queries, and property registries. The scene module wraps
the ECS, depending on math for transforms and exposing entity handles consumed by animation, physics, and
rendering.

### ECS Layering

The [`Scene` façade](engine/scene/include/engine/scene/scene.hpp) wraps an `entt::registry` and owns
entity creation, destruction, and component storage. High-level systems interact exclusively through the
`Scene` interface—either by iterating `Scene::view<Components...>()` or by caching `Entity` handles that
validate themselves before acting. This design enforces a clear ownership model: only the scene owns the
registry, while other modules supply components and systems that operate on it. Future subsystem
initializers can extend `Scene::initialize_systems()` without leaking `entt` internals across module
boundaries.

### Runtime Subsystem Discovery

The runtime module surfaces all loadable subsystems. Its
[`api.cpp`](engine/runtime/src/api.cpp) queries a configurable `SubsystemRegistry` to populate
module identifiers and expose them through the exported C ABI (`engine_runtime_module_*`). Hosts can
toggle availability at build time via `ENGINE_ENABLE_<MODULE>` flags or at runtime by calling the helper
configuration APIs in `engine/runtime/api.hpp`. Extending the registry is sufficient for new modules to
appear in the discovery list, keeping runtime configuration declarative and centralised.

## API Surface Summary

The API documentation collects subsystem summaries and call sequences that mirror the exported headers.
Update the relevant notes whenever a public header changes and keep diagrams or tables close to the code
they describe to minimise drift. Highlights per module:

### Animation

The animation API exposes lightweight structures used to author and evaluate skeletal clips at runtime.
Public declarations live in
[`engine/animation/api.hpp`](engine/animation/include/engine/animation/api.hpp).

- `JointPose` – Stores translation, rotation (quaternion), and scale for an individual joint.
- `Keyframe` / `JointTrack` – Timestamped pose samples for a named joint.
- `AnimationClip` – Aggregates tracks and records the clip duration.
- `AnimationController` – Tracks playback state (time, speed, looping) for a clip.
- `AnimationBlendTree` – Node-based graph that evaluates clips and blend operators.
- `BlendTreeParameter` – Runtime parameter binding (float, bool, event) surfaced to the host application.
- `AnimationRigPose` – Container of joint poses with constant-time name lookup via `find()`.

```cpp
#include <engine/animation/api.hpp>

auto clip = engine::animation::make_default_clip();
auto controller = engine::animation::make_linear_controller(std::move(clip));
engine::animation::advance_controller(controller, 0.016);
auto pose = engine::animation::evaluate_controller(controller);
```

`make_default_clip()` returns a procedural oscillation clip used by the runtime smoke test.
`make_linear_controller` validates keyframe ordering and normalises the clip duration before returning a
controller ready for playback. `AnimationRigPose::find("root")` returns the active joint pose; the physics
and geometry subsystems consume it to influence forces and mesh deformation.

Blend trees expose composable nodes that mix clips and controller output based on parameter binding:

```cpp
using namespace engine::animation;

AnimationBlendTree tree;
auto idle = add_clip_node(tree, load_clip("idle.json"));
auto walk = add_clip_node(tree, load_clip("walk.json"));
auto blend = add_linear_blend_node(tree, idle, walk, 0.5f);
set_blend_tree_root(tree, blend);

auto speed_param = add_float_parameter(tree, "speed", 0.0f);
bind_linear_blend_weight(tree, blend, speed_param);

advance_blend_tree(tree, dt);
auto pose = evaluate_blend_tree(tree);
```

`add_clip_node` stores the clip handle in the tree and returns a node identifier.
`add_linear_blend_node` mixes two input nodes according to a bound parameter or constant weight. Parameters
act as the runtime control surface and advance alongside the blend tree so callers can drive weights,
events, and boolean gates in lockstep with the host application.
`add_additive_blend_node` layers a delta pose over a base node, interpreting the additive input relative to the identity pose and
scaling it by a float weight (direct value or bound parameter) before composing translation, rotation, and scale onto the base.

### Compute

The compute API implements a CPU-oriented kernel dispatcher mirroring the dependency management used by GPU
command graphs. Headers live in
[`engine/compute/api.hpp`](engine/compute/include/engine/compute/api.hpp). `KernelDispatcher` records named
kernels and their dependencies. `dispatch()` performs a Kahn topological sort, executes each kernel exactly
once, and returns an `ExecutionReport` containing the realised execution order together with per-kernel
durations:

```cpp
#include <engine/compute/api.hpp>

engine::compute::KernelDispatcher dispatcher;
const auto prepare = dispatcher.add_kernel("prepare", [] {
    // upload data, allocate buffers, etc.
});
dispatcher.add_kernel("simulate", [] {
    // run the heavy work
}, {prepare});
const auto report = dispatcher.dispatch();
for (std::size_t i = 0; i < report.execution_order.size(); ++i) {
    std::cout << report.execution_order[i] << " took "
              << report.kernel_durations[i] << "s\n";
}
```

If a dependency index is invalid or a cycle is detected, the dispatcher raises an exception to prevent
inconsistent runtime state. The runtime module uses the report to surface kernel ordering and timing through
the C API.

### Geometry

The geometry module layers geometric primitives, spatial queries, and property management utilities on top
of the core math types. Public headers live under
[`engine/geometry/include`](engine/geometry/include).

- [`engine/geometry/shapes/aabb.hpp`](engine/geometry/include/engine/geometry/shapes/aabb.hpp) introduces
  axis-aligned bounding boxes with helpers for bounding-volume hierarchy construction and intersection
  queries.
- [`engine/geometry/shapes.hpp`](engine/geometry/include/engine/geometry/shapes.hpp) aggregates canonical
  shape definitions so call sites can pull the entire catalogue with a single include.
- [`engine/geometry/utils/shape_interactions.hpp`](engine/geometry/include/engine/geometry/utils/shape_interactions.hpp)
  enumerates robust intersection tests with configurable epsilon tolerances.
- [`engine/geometry/utils/connectivity.hpp`](engine/geometry/include/engine/geometry/utils/connectivity.hpp)
  provides mesh traversal helpers, circulators, and range adaptors for half-edge data structures.
- [`engine/geometry/random.hpp`](engine/geometry/include/engine/geometry/random.hpp) defines deterministic
  sampling APIs for scattering points or randomising bounding volumes.
- [`engine/geometry/properties/property_set.hpp`](engine/geometry/include/engine/geometry/properties/property_set.hpp)
  and [`property_registry.hpp`](engine/geometry/include/engine/geometry/properties/property_registry.hpp)
  manage per-element attributes with type-erased buffers and handle-based access.

`engine/geometry/api.hpp` exposes a compact `SurfaceMesh` struct geared towards runtime deformation. It
stores rest-state and deformed vertex positions, indices, normals, and cached bounds. Key helpers include
`make_unit_quad()`, `load_surface_mesh`, `save_surface_mesh`, `apply_uniform_translation`,
`recompute_vertex_normals`, `update_bounds`, and `centroid`. These functions allow the runtime to preview
animation- and physics-driven deformation without touching the heavier half-edge mesh infrastructure.

### Math

The math module provides the foundational numeric types and routines that power higher-level systems. Its
public headers live under [`engine/math/include`](engine/math/include) and expose:

- [`engine/math/common.hpp`](engine/math/include/engine/math/common.hpp) – compile-time traits, literal
  helpers, and inline macros.
- [`engine/math/vector.hpp`](engine/math/include/engine/math/vector.hpp) – templated `Vector<T, N>`
  container with arithmetic operators and dimension-changing constructors.
- [`engine/math/matrix.hpp`](engine/math/include/engine/math/matrix.hpp) – column-major matrices with proxy
  types, arithmetic composition, and conversion helpers.
- [`engine/math/quaternion.hpp`](engine/math/include/engine/math/quaternion.hpp) – Hamiltonian quaternion
  implementation with scalar/vector constructors and product operators.
- [`engine/math/utils.hpp`](engine/math/include/engine/math/utils/utils.hpp) – inline utilities for
  clamping, extrema queries, and tolerance-based comparisons.
- [`engine/math/utils_rotation.hpp`](engine/math/include/engine/math/utils/utils_rotation.hpp) – rotation
  helpers supplementing quaternion and matrix math.

All math types are header-only templates. Constructors favour explicit semantics to avoid accidental
narrowing, and the module intentionally avoids heap allocation for suitability in real-time systems.

### Physics

The physics API provides a compact `PhysicsWorld`/`RigidBody` pair for deterministic, step-based simulation.
Public interfaces are defined in
[`engine/physics/api.hpp`](engine/physics/include/engine/physics/api.hpp).

```cpp
#include <engine/physics/api.hpp>

engine::physics::PhysicsWorld world;
world.gravity = {0.0F, -9.81F, 0.0F};
engine::physics::RigidBody body;
body.mass = 2.0F;
const auto handle = engine::physics::add_body(world, body);
engine::physics::apply_force(world, handle, {0.0F, 20.0F, 0.0F});
engine::physics::integrate(world, 0.016);
```

`RigidBody` stores mass, inverse mass, position, velocity, and an accumulated force. `add_body` normalises
the mass field and appends the body to the world. `body_at(world, index)` throws `std::out_of_range` when
the index is invalid, surfacing configuration mistakes early. `clear_forces` zeroes accumulators when
callers need a clean slate before the next update.

### Runtime

The runtime module aggregates subsystem entry points and exposes a stable C ABI for dynamic discovery.
[`engine/runtime/api.hpp`](engine/runtime/include/engine/runtime/api.hpp) defines the public surface:

- `initialize()` / `shutdown()` – Manage lifetime of the shared simulation state.
- `runtime_frame_state tick(double dt)` – Steps animation, compute, physics, and geometry in a deterministic
  order and returns the resulting pose, body positions, bounds, scene graph snapshot, and compute execution
  report with per-kernel timings.
- `const geometry::SurfaceMesh& current_mesh()` – Provides direct access to the deformed mesh for
  inspection.

The returned `runtime_frame_state::dispatch_report.execution_order` mirrors the ordering produced by
`compute::KernelDispatcher`, and `kernel_durations` expose wall-clock timings for profiling overlays. The C
bindings mirror these helpers (`engine_runtime_initialize`, `engine_runtime_tick`, body/joint accessors,
dispatch inspection, and scene graph queries), enabling scripting layers such as the Python bindings in
`python/engine3g/loader.py` to coordinate the simulation.

### Scene

The scene module adapts [`entt`](https://github.com/skypjack/entt) into an engine-friendly façade. The
[`Scene`](engine/scene/include/engine/scene/scene.hpp) and `Entity` wrappers manage lifetime, component
insertion, and registry access. Inline implementations forward to the underlying `entt::registry`, providing
helpers such as `Entity::emplace`, `Entity::destroy`, and templated `Scene::view` iterators. `Scene`
instances own an `entt::registry` and control system initialisation via an internal `initialize_systems`
hook. Entities are lightweight handles that validate against their backing scene before performing registry
operations, preventing accidental use-after-destroy patterns. Aside from the ABI surface, the module is
header-only, enabling inline component access in performance-critical loops.

## Build Targets and Dependency Graph

This inventory enumerates the CMake targets that originate from the root build and the `engine/**/CMakeLists`
hierarchy. The goal is to make the dependency structure explicit and highlight gaps or potential refactors
before introducing additional subsystems.

The root build defines two cross-cutting interface targets:

- `engine::project_options` – Propagates the required C++20 feature set and compiler-specific switches such
  as `-stdlib=libc++` when building with Clang.
- `engine::headers` – Aggregates the public header directories registered by each engine module so that
  tooling, integration tests, or external consumers can attach to the entire header surface with a single
  dependency.

All module libraries link against `engine::project_options`, and each module registers its public include
directory through `engine_apply_module_defaults`, which simultaneously forwards that directory to
`engine::headers`.

### Engine Libraries

| Target | Type | Direct Dependencies |
| --- | --- | --- |
| `engine_animation` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_assets` | SHARED/STATIC | `engine::project_options`, `engine_io` |
| `engine_compute` | SHARED/STATIC | `engine::project_options`, `engine_math`, `engine_compute_cuda` |
| `engine_compute_cuda` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_core` | SHARED/STATIC | `engine::project_options`, `EnTT::EnTT`, `spdlog::spdlog_header_only`, `imgui::imgui` |
| `engine_geometry` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_io` | SHARED/STATIC | `engine::project_options`, `engine_geometry` |
| `engine_math` | INTERFACE | `engine::project_options` |
| `engine_physics` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_platform` | SHARED/STATIC | `engine::project_options`, `glfw` *or* `glfw_shared` (configure-time choice) |
| `engine_rendering` | SHARED/STATIC | `engine::project_options`, `engine_core`, `engine_assets`, `engine_platform`, `engine_scene` |
| `engine_runtime` | SHARED/STATIC | `engine::project_options`, `engine_animation`, `engine_assets`, `engine_compute`, `engine_compute_cuda`, `engine_core`, `engine_geometry`, `engine_io`, `engine_math`, `engine_physics`, `engine_platform`, `engine_rendering`, `engine_scene` |
| `engine_scene` | SHARED/STATIC | `engine::project_options`, `EnTT::EnTT`, `engine_core`, `engine_math` |

`engine::headers` collects the include directories contributed by each module (including
`engine_compute_cuda`). Consumers that only need the headers can link to this interface target without
incurring object code dependencies.

### Tests and Utilities

Module-specific test executables share a common shape: they link against the corresponding module library,
the GoogleTest main target, and `engine::project_options` to inherit the compile feature requirements.

| Target | Purpose |
| --- | --- |
| `engine_animation_tests` | Animation unit tests |
| `engine_assets_tests` | Asset pipeline smoke tests |
| `engine_compute_tests` | CPU compute façade tests |
| `engine_compute_cuda_tests` | CUDA compute façade tests |
| `engine_core_tests` | ECS and core service tests |
| `engine_geometry_tests` | Geometry primitives and utilities |
| `engine_geometry_shape_interactions_tests` | Focused geometry interaction suite |
| `engine_io_tests` | IO registries |
| `engine_math_tests` | Header-only math validation |
| `engine_physics_tests` | Physics utilities |
| `engine_platform_tests` | Windowing abstractions |
| `engine_platform_window_app` | Manual smoke harness for the platform layer |
| `engine_rendering_tests` | Rendering orchestrator checks |
| `engine_runtime_tests` | Runtime aggregation |
| `engine_scene_tests` | Scene graph tests |

The root build also defines the `docs` custom target that drives `scripts/validate_docs.py` when Python is
available.

### Third-Party Integrations

Third-party libraries are brought in through their own packages (`EnTT::EnTT`, `spdlog::spdlog_header_only`,
`imgui::imgui`, `glfw`/`glfw_shared`) and remain isolated from engine internals except through explicit
`target_link_libraries` usage requirements. FetchContent fallbacks for EnTT and GLFW honour the cache
variables emitted by the presets, ensuring consistent feature toggles regardless of whether dependencies are
vendored or fetched at configure time.

### Identified Gaps

1. **CUDA hard dependency** – `engine_compute` and `engine_runtime` always pull in `engine_compute_cuda`.
   Introducing a configuration option would let CPU-only builds skip CUDA entirely.
2. **Platform backend selection** – The build assumes GLFW is the active windowing provider. SDL stubs exist
   in the source tree, but no preset toggle guards their inclusion.
3. **Header aggregation visibility** – While `engine::headers` simplifies discovery for tooling, external
   consumers that link only a subset of modules may inadvertently gain include visibility into unrelated
   subsystems. Monitoring include hygiene in downstream targets is recommended.

The dependency graph contains no cycles. The longest chains currently flow from
`engine_runtime` → `engine_compute` → `engine_compute_cuda` → `engine_math` and from
`engine_assets` → `engine_io` → `engine_geometry` → `engine_math`.

## Rendering Module Include Path Audit

- Synchronisation primitives now live in `engine/rendering/include/engine/rendering/resources`, so the
  public API no longer relies on relative includes.
- `engine_apply_module_defaults` exposes only the `include/` subtree to dependants, keeping backend and
  test-only headers private.
- Public headers consumed by downstream clients all reside under `engine/rendering/include`; backend
  schedulers and test helpers remain implementation details.

Details:

- `engine_apply_module_defaults` adds the values passed in `PUBLIC_INCLUDE_DIRS` to the module target and to
  the aggregate interface target `engine::headers`. The rendering module contributes only its `include`
  directory, restricting exported paths.
- `PRIVATE_INCLUDE_DIRS` affects only module compilation. Supplying `${CMAKE_SOURCE_DIR}` in the rendering
  module lets implementation files see the whole source tree without propagating that path to consumers.
- `engine/rendering/include/engine/rendering/gpu_scheduler.hpp` and `frame_graph.hpp` include
  `engine/rendering/resources/synchronization.hpp` directly now that the header sits under the exported
  include tree.
- Headers outside `engine/rendering/include` (`engine/rendering/backend/*.hpp`,
  `engine/rendering/tests/scheduler_test_utils.hpp`) are confined to backend scaffolding and test utilities.

- `engine/rendering/backend/vulkan/resource_translation.hpp` now exposes helper utilities that map
  `FrameGraphResourceInfo` descriptors and synchronisation barriers into Vulkan create-info structures.
  Frame-graph resources require explicit extent, mip-count, array-layer, sample-count, and buffer-size
  metadata; these fields are serialized alongside existing format/usage/state data and validated during
  compilation. See `engine/rendering/tests/test_vulkan_resource_translation.cpp` for example usage and
  expected Vulkan mappings.

Recommendation: continue publishing consumer-facing headers from `engine/rendering/include` and keep
backend/test helpers private. When future resource abstractions need to be shared, colocate them alongside
the synchronisation primitives to avoid reintroducing relative includes.

## Engineering Notes — README Reconnaissance (2025-02-14)

Context: audited the repository root `README.md` to extract build, testing, and layout guidance.

Open questions / gaps:

1. **Toolchain specificity** — The build section enumerates the CMake invocation but omits required
   compiler versions, SDK dependencies (e.g., Vulkan SDK), or minimum CMake version. Clarify to guarantee
   reproducible builds across platforms.
2. **Python environment** — `pytest` validates `engine3g.loader`, yet the expected virtual environment,
   required packages, and binding generation steps remain undocumented.
3. **Third-party updates** — Vendored library update cadence and contribution workflow are unstated.
4. **Testing scope** — The README does not list existing test suites or quality gates such as coverage or
   sanitiser requirements. Align on expectations.
5. **Feature roadmap prioritisation** — The TODO backlog is exhaustive but unordered. Product guidance is
   required to understand in-flight milestone focus.

Next steps:

- Sync with build/release owners to enumerate minimum supported toolchains and platform-specific caveats.
- Draft a Python environment setup guide (potentially under `python/README.md`) once dependencies are
  confirmed.
- Propose documentation updates detailing third-party maintenance responsibilities.
- Audit existing tests to classify by subsystem and recommend coverage metrics.
- Request product/tech-lead input on TODO sequencing for roadmap planning.

## Roadmap Alignment

The consolidated sequencing across major subsystems lives in
[`docs/ROADMAP.md`](docs/ROADMAP.md). The roadmap is ultimately aimed at delivering an engine that makes advanced yet accessible graphics and geometry processing research straightforward, with every required tool slated for future implementation. Near-term priorities focus on:

- Extending animation with deformation bindings and richer parameter binding now that clip
  validation, serialization, and blend-tree evaluation are in place.
- Building contact manifolds, constraint hooks, and instrumentation on top of the existing
  physics broad phase and collider coverage.
- Enriching rendering resource descriptors and GPU scheduler plumbing ahead of backend
  integrations.
- Finishing geometry/IO round-tripping for meshes, point clouds, and graphs so deformation
  and collision pipelines can rely on shared data structures.
- Hardening `RuntimeHost` lifecycle diagnostics and scene mirroring before layering on
  streaming and render submission.
- Replacing platform mock backends with GLFW/SDL integrations and surfacing filesystem
  write/watch utilities to unblock hot reload and tooling.

## TODO / Next Steps

### Aggregated README Backlog

Keep this table aligned with the per-module roadmaps and the global alignment
summary referenced above.

| Module | Near-term focus | Source |
| --- | --- | --- |
| Animation | Harden linear blend skinning with telemetry and authoring guidance now that runtime deformation is wired through the controller/blend-tree stack. | [docs/modules/animation/README.md](docs/modules/animation/README.md)<br>[docs/modules/animation/ROADMAP.md](docs/modules/animation/ROADMAP.md) |
| Assets | Define authoritative asset metadata, stage asynchronous streaming task graphs, and persist cache artefacts with hot-reload diagnostics. | [docs/modules/assets/README.md](docs/modules/assets/README.md)<br>[docs/modules/assets/ROADMAP.md](docs/modules/assets/ROADMAP.md) |
| Compute | Leverage the new dependency graph diagnostics to harden scheduling and integrate a configurable clock abstraction so execution reports capture CPU vs GPU timing domains ahead of the CUDA executor expansion (`AI-004`). | [docs/modules/compute/README.md](docs/modules/compute/README.md)<br>[docs/modules/compute/ROADMAP.md](docs/modules/compute/ROADMAP.md) |
| Core | Establish application lifecycle/configuration/diagnostics services and thread them through the runtime façade with expanded test coverage. | [docs/modules/core/README.md](docs/modules/core/README.md)<br>[docs/modules/core/ROADMAP.md](docs/modules/core/ROADMAP.md) |
| Geometry & IO | Consolidate bounds/naming across geometry headers while finishing graph/mesh/point-cloud pipelines and enriching import/export diagnostics ahead of v2.0. | [docs/modules/geometry/README.md](docs/modules/geometry/README.md)<br>[docs/modules/geometry/ROADMAP.md](docs/modules/geometry/ROADMAP.md)<br>[docs/modules/io/README.md](docs/modules/io/README.md)<br>[docs/modules/io/ROADMAP.md](docs/modules/io/ROADMAP.md) |
| Math | Document public headers, broaden decomposition/numerics support, and raise regression coverage around existing primitives. | [docs/modules/math/README.md](docs/modules/math/README.md)<br>[docs/modules/math/ROADMAP.md](docs/modules/math/ROADMAP.md) |
| Physics | Build on the rigid-body, collider, and sweep-and-prune foundation by adding contact manifolds, constraint solving, and instrumentation. | [docs/modules/physics/README.md](docs/modules/physics/README.md)<br>[docs/modules/physics/ROADMAP.md](docs/modules/physics/ROADMAP.md) |
| Platform | Wire up concrete GLFW/SDL detection on top of the configurable backend selector, add filesystem write/watch utilities, and surface real input device plumbing. | [docs/modules/platform/README.md](docs/modules/platform/README.md)<br>[docs/modules/platform/ROADMAP.md](docs/modules/platform/ROADMAP.md) |
| Rendering | Enrich frame-graph resource descriptors, thread queue/command metadata, and prototype the reference GPU scheduler before wiring backends. | [docs/modules/rendering/README.md](docs/modules/rendering/README.md)<br>[docs/modules/rendering/ROADMAP.md](docs/modules/rendering/ROADMAP.md) |
| Runtime | Use the new `RuntimeHost::diagnostics()` telemetry to monitor lifecycle performance while tightening dependency reset validation ahead of the streaming hooks. | [docs/modules/runtime/README.md](docs/modules/runtime/README.md)<br>[docs/modules/runtime/ROADMAP.md](docs/modules/runtime/ROADMAP.md)<br>[docs/ROADMAP.md#runtime-expansion-plan](docs/ROADMAP.md#runtime-expansion-plan) |
| Scene | Define schemas for core runtime components, broaden traversal helpers, and version the serialization format alongside expanded tests. | [docs/modules/scene/README.md](docs/modules/scene/README.md)<br>[docs/modules/scene/ROADMAP.md](docs/modules/scene/ROADMAP.md) |
| Tools | Stand up shared tooling infrastructure, automate content pipelines, surface profiling flows, and iterate towards the editor shell. | [docs/modules/tools/README.md](docs/modules/tools/README.md)<br>[docs/modules/tools/ROADMAP.md](docs/modules/tools/ROADMAP.md) |
