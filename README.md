# Test Engine Workspace

## Current State

- Modular real-time engine prototype written in modern C++20 with optional Python tooling for automation and validation.
- Subsystems live under `engine/` and build as individual libraries (animation, compute, geometry, physics, rendering, runtime, scene, tools, and platform glue).
- Documentation under `docs/` captures design decisions and API notes, while `python/` and `scripts/` provide automation entry points.

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
cmake -S . -B build -G Ninja
cmake --build build
```

Each subsystem produces a library named `engine_<subsystem>`; the runtime aggregates them via `engine_runtime`.

### Testing

- **C++ suites** – Configure with `-DBUILD_TESTING=ON` and run `ctest --test-dir build`.
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

## TODO / Next Steps

### Aggregated README Backlog

| TODO / Next Steps | Modules |
| --- | --- |
| Add blend-tree authoring and clip serialization to unlock complex rigs. | engine/animation/README.md |
| Add collision detection, constraints, and stable integration schemes. | engine/physics/README.md |
| Add format importers to translate real data into runtime structures. | engine/io/importers/README.md |
| Add platform-aware build presets and dependency bootstrapping scripts. | scripts/build/README.md |
| Add scenario-driven examples and profiling to exercise the implementation. | engine/animation/src/README.md<br>engine/assets/src/README.md<br>engine/compute/src/README.md<br>engine/core/ecs/README.md<br>engine/core/src/README.md<br>engine/geometry/src/README.md<br>engine/geometry/src/shapes/README.md<br>engine/io/src/README.md<br>engine/physics/src/README.md<br>engine/platform/src/README.md<br>engine/rendering/src/README.md<br>engine/runtime/src/README.md<br>engine/scene/src/README.md |
| Author representative shader assets once the rendering pipeline is defined. | engine/assets/shaders/README.md<br>engine/rendering/materials/shaders/README.md<br>engine/rendering/materials/shaders/common/README.md<br>engine/rendering/materials/shaders/glsl/README.md<br>engine/rendering/materials/shaders/hlsl/README.md |
| Bind platform abstractions to OS windowing, input, and filesystem APIs. | engine/platform/README.md |
| Broaden math primitives and numerics beyond the current minimal set. | engine/math/README.md |
| Build content pipeline automation that feeds assets into the engine. | engine/tools/pipelines/README.md |
| Build scene graph traversal and dependency evaluation. | engine/scene/graph/README.md |
| Capture design rationales and outcomes for upcoming milestones. | docs/design/README.md |
| Connect the frame graph to GPU submission and synchronization. | engine/rendering/pipeline/README.md |
| Connect the frame graph to concrete GPU backends and resource providers. | engine/rendering/README.md |
| Consolidate automation entry points for developers and CI environments. | scripts/README.md |
| Create exporters to persist runtime assets back to disk. | engine/io/exporters/README.md |
| Curate representative scenarios across unit, integration, and performance suites. | engine/tests/README.md |
| Define component schemas and authoring utilities for the runtime. | engine/scene/components/README.md |
| Design asset pipelines that feed runtime modules beyond sample data. | engine/assets/README.md |
| Design render passes and pipelines once a resource provider is available. | engine/rendering/passes/README.md |
| Develop the editor shell and hook it into the runtime subsystems. | engine/tools/editor/README.md |
| Document subsystem APIs and keep diagrams in sync with code changes. | docs/api/README.md |
| Document the public headers once the API stabilises. | engine/animation/include/engine/animation/README.md<br>engine/assets/include/engine/assets/README.md<br>engine/compute/cuda/include/engine/compute/cuda/README.md<br>engine/compute/include/engine/compute/README.md<br>engine/core/include/engine/core/README.md<br>engine/geometry/include/engine/geometry/README.md<br>engine/geometry/include/engine/geometry/shapes/README.md<br>engine/io/include/engine/io/README.md<br>engine/math/include/engine/math/README.md<br>engine/physics/include/engine/physics/README.md<br>engine/platform/include/engine/platform/README.md<br>engine/rendering/include/engine/rendering/README.md<br>engine/runtime/include/engine/runtime/README.md<br>engine/scene/include/engine/scene/README.md |
| Drive subsystem lifecycles and state management for end-to-end scenarios. | engine/runtime/README.md |
| Expand living design documents with current architectural decisions. | docs/README.md |
| Expand regression coverage beyond the current smoke checks. | engine/animation/tests/README.md<br>engine/assets/tests/README.md<br>engine/compute/cuda/tests/README.md<br>engine/compute/tests/README.md<br>engine/core/tests/README.md<br>engine/geometry/tests/README.md<br>engine/io/tests/README.md<br>engine/math/tests/README.md<br>engine/physics/tests/README.md<br>engine/platform/tests/README.md<br>engine/rendering/tests/README.md<br>engine/runtime/tests/README.md<br>engine/scene/tests/README.md<br>engine/tests/integration/README.md<br>engine/tests/performance/README.md<br>engine/tests/unit/README.md |
| Extend engine core services for application control, configuration, and diagnostics. | engine/core/README.md |
| Harden the loader API and expose ergonomic runtime bindings. | python/engine3g/README.md |
| Hook materials into the rendering backend and asset pipeline. | engine/rendering/materials/README.md |
| Implement CI harness scripts mirroring the documented workflow. | scripts/ci/README.md |
| Implement GPU resource management for buffers, textures, and samplers. | engine/rendering/resources/README.md<br>engine/rendering/resources/buffers/README.md<br>engine/rendering/resources/samplers/README.md |
| Implement GPU-backed dispatch and integrate with runtime scheduling. | engine/compute/README.md |
| Implement actual input device polling per platform. | engine/platform/input/README.md |
| Implement lighting models and data flows for the renderer. | engine/rendering/lighting/README.md |
| Implement persistent cache strategies for imported assets. | engine/io/cache/README.md |
| Implement scene graph serialization, systems, and runtime traversal. | engine/scene/README.md |
| Implement scene serialization and deserialization compatible with runtime assets. | engine/scene/serialization/README.md |
| Implement scene systems that operate over ECS data during the frame. | engine/scene/systems/README.md |
| Implement the actual backend integration using the target API. | engine/rendering/backend/README.md<br>engine/rendering/backend/directx12/README.md<br>engine/rendering/backend/metal/README.md<br>engine/rendering/backend/opengl/README.md<br>engine/rendering/backend/vulkan/README.md |
| Implement visibility determination algorithms to drive culling. | engine/rendering/visibility/README.md |
| Instrument and visualise profiling data for frame and job execution. | engine/tools/profiling/README.md |
| Integrate CUDA kernels and scheduling into the dispatcher. | engine/compute/cuda/README.md<br>engine/compute/cuda/src/README.md |
| Integrate local and virtual filesystem providers for asset loading. | engine/platform/filesystem/README.md |
| Introduce advanced mesh processing (remeshing, parameterization, collision prep). | engine/geometry/README.md |
| Populate this placeholder directory with its intended implementation when scoped. | engine/README.md<br>engine/core/application/README.md<br>engine/core/configuration/README.md<br>engine/core/diagnostics/README.md<br>engine/core/memory/README.md<br>engine/core/parallel/README.md<br>engine/core/plugin/README.md<br>engine/core/runtime/README.md<br>engine/geometry/csg/README.md<br>engine/geometry/decimation/README.md<br>engine/geometry/mesh/README.md<br>engine/geometry/surfaces/README.md<br>engine/geometry/topology/README.md<br>engine/geometry/uv/README.md<br>engine/geometry/volumetric/README.md<br>engine/physics/collision/README.md<br>engine/physics/dynamics/README.md<br>engine/rendering/materials/textures/README.md |
| Prototype deformation and rigging utilities for the animation pipeline. | engine/animation/deformation/README.md<br>engine/animation/rigging/README.md<br>engine/animation/skinning/README.md |
| Provide curated sample assets demonstrating runtime features. | engine/assets/samples/README.md |
| Publish the Python packaging and dependency manifest for automation helpers. | python/README.md |
| Replace the stub window factories with real OS backends. | engine/platform/windowing/README.md |
| Stabilise cross-module integration and build targets for contributors. | README.md |
| Stand up tooling flows for content pipelines, profiling, and editor workflows. | engine/tools/README.md |
| Wire format import/export and caching to serve real content pipelines. | engine/io/README.md |

### Updating This Table

Regenerate the per-directory README files (for example via `python generate_readmes.py`) and copy the resulting aggregation to keep the backlog synchronised.
