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
| Assets | Runtime caches for meshes, point clouds, materials, shaders, and textures with hot-reload polling and handle-based lookups. |
| Compute | A topological kernel dispatcher that measures execution time per kernel and exposes a lightweight math helper for identity transforms. |
| Core | EnTT-backed registry façade, entity helpers, and module discovery utilities that higher-level systems consume. |
| Geometry | `SurfaceMesh` utilities (normals, bounds, centroid), conversions to/from the halfedge core, procedural primitives, and ASCII import/export for meshes, graphs, and point clouds backed by spatial indices such as kd-trees and octrees. |
| IO | Geometry and animation import/export wrappers with format detection, a plugin-ready registry for mesh/point-cloud/graph handlers, and scaffolding for cache policies. |
| Math | Vector, matrix, quaternion, and transform primitives plus orthonormal basis helpers that feed animation, geometry, and physics. |
| Physics | Rigid-body world with mass clamping, damping, configurable substepping, sphere/AABB/capsule colliders, and sweep-and-prune broad-phase collision detection. |
| Platform | Virtual filesystem providers, backend selection plumbing, and mocked window/input services pending concrete OS integrations. |
| Rendering | Frame-graph compilation/execution, command encoder hooks, resource lifetime tracking, and a backend-neutral GPU scheduler interface. |
| Runtime | `RuntimeHost` orchestration that advances animation, drives physics via the compute dispatcher, deforms geometry, updates bounds, and mirrors joint transforms into an EnTT-backed scene. |
| Scene | Entity façade with hierarchy and transform propagation systems, deterministic serialization/deserialization, and component helpers used by the runtime. |
| Tools | Staging area for editor, profiling, and pipeline automation features with roadmap-driven scaffolding. |

### Module Overview

- **`docs/`** – Centralised design records, API notes, and the reusable README template.
- **`engine/`** – Native subsystems organised by responsibility with matching tests and headers.
- **`python/`** – Loader and helper code that interacts with the compiled engine modules.
- **`scripts/`** – Build, validation, and CI orchestration entry points.
- **`third_party/`** – Vendored dependencies such as EnTT, Dear ImGui, spdlog, and GoogleTest.

## Usage

### Prerequisites

- **Compilers** – C++20-capable toolchain (MSVC 19.3x, Clang 15+, or GCC 12+).
- **Build system** – CMake 3.20+ (per `cmake_minimum_required`) and Ninja or another generator.
- **Python** – Python 3.12+ with `pip` for scripts and test harnesses.
- **Host libraries** – Platform SDKs/drivers for the rendering backends you plan to target (Vulkan SDK, DirectX 12 Agility SDK, or system OpenGL drivers). Linux builds that enable GLFW require `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev`.

### Configure and Build

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Presets live under `scripts/build/` and currently cover Linux (GCC) and Windows (MSVC) compiler stacks. Additional variants can be invoked with `cmake --preset <name>` or orchestrated collectively via `scripts/ci/run_presets.py`. Each subsystem still produces a library named `engine_<subsystem>`; linking to any of them automatically imports the shared usage requirements published by `engine::project_options` and the aggregated headers exposed through `engine::headers`.

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

## Global Roadmap Alignment

The consolidated sequencing across major subsystems lives in
[`docs/global_roadmap.md`](docs/global_roadmap.md). The roadmap is ultimately aimed at delivering an engine that makes advanced yet accessible graphics and geometry processing research straightforward, with every required tool slated for future implementation. Near-term priorities focus on:

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
| Animation | Layer additive and deformation-aware blend nodes on top of the existing controller/blend-tree stack while expanding profiling coverage. | [docs/modules/animation/README.md](docs/modules/animation/README.md)<br>[docs/modules/animation/ROADMAP.md](docs/modules/animation/ROADMAP.md) |
| Assets | Define authoritative asset metadata, stage import task graphs, and persist cache artefacts with hot-reload diagnostics. | [docs/modules/assets/README.md](docs/modules/assets/README.md)<br>[docs/modules/assets/ROADMAP.md](docs/modules/assets/ROADMAP.md) |
| Compute | Unify dispatch descriptions so CPU/GPU executors share infrastructure, then stand up the CUDA path and runtime integration. | [docs/modules/compute/README.md](docs/modules/compute/README.md)<br>[docs/modules/compute/ROADMAP.md](docs/modules/compute/ROADMAP.md) |
| Core | Establish application lifecycle/configuration/diagnostics services and thread them through the runtime façade with expanded test coverage. | [docs/modules/core/README.md](docs/modules/core/README.md)<br>[docs/modules/core/ROADMAP.md](docs/modules/core/ROADMAP.md) |
| Geometry & IO | Consolidate bounds/naming across geometry headers while finishing graph/mesh/point-cloud pipelines and enriching import/export diagnostics ahead of v2.0. | [docs/modules/geometry/README.md](docs/modules/geometry/README.md)<br>[docs/modules/geometry/ROADMAP.md](docs/modules/geometry/ROADMAP.md)<br>[docs/modules/io/README.md](docs/modules/io/README.md)<br>[docs/modules/io/ROADMAP.md](docs/modules/io/ROADMAP.md) |
| Math | Document public headers, broaden decomposition/numerics support, and raise regression coverage around existing primitives. | [docs/modules/math/README.md](docs/modules/math/README.md)<br>[docs/modules/math/ROADMAP.md](docs/modules/math/ROADMAP.md) |
| Physics | Build on the rigid-body, collider, and sweep-and-prune foundation by adding contact manifolds, constraint solving, and instrumentation. | [docs/modules/physics/README.md](docs/modules/physics/README.md)<br>[docs/modules/physics/ROADMAP.md](docs/modules/physics/ROADMAP.md) |
| Platform | Replace mock backends with GLFW/SDL integrations, add filesystem write/watch utilities, and surface real input device plumbing. | [docs/modules/platform/README.md](docs/modules/platform/README.md)<br>[docs/modules/platform/ROADMAP.md](docs/modules/platform/ROADMAP.md) |
| Rendering | Enrich frame-graph resource descriptors, thread queue/command metadata, and prototype the reference GPU scheduler before wiring backends. | [docs/modules/rendering/README.md](docs/modules/rendering/README.md)<br>[docs/modules/rendering/ROADMAP.md](docs/modules/rendering/ROADMAP.md) |
| Runtime | Extend `RuntimeHost` diagnostics, dispatcher programmability, and streaming hooks to support end-to-end orchestration scenarios. | [docs/modules/runtime/README.md](docs/modules/runtime/README.md)<br>[docs/modules/runtime/ROADMAP.md](docs/modules/runtime/ROADMAP.md)<br>[docs/design/runtime_plan.md](docs/design/runtime_plan.md) |
| Scene | Define schemas for core runtime components, broaden traversal helpers, and version the serialization format alongside expanded tests. | [docs/modules/scene/README.md](docs/modules/scene/README.md)<br>[docs/modules/scene/ROADMAP.md](docs/modules/scene/ROADMAP.md) |
| Tools | Stand up shared tooling infrastructure, automate content pipelines, surface profiling flows, and iterate towards the editor shell. | [docs/modules/tools/README.md](docs/modules/tools/README.md)<br>[docs/modules/tools/ROADMAP.md](docs/modules/tools/ROADMAP.md) |
