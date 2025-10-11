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

## TODO / Next Steps

### Aggregated README Backlog

| TODO / Next Steps | Modules |
| --- | --- |
| Add blend-tree authoring and clip serialization to unlock complex rigs. | [engine/animation/README.md](engine/animation/README.md)<br>[docs/animation_roadmap.md](docs/animation_roadmap.md) |
| Execute the physics roadmap: stabilise the core world, add scalable collision management, and broaden collider coverage. | [engine/physics/README.md](engine/physics/README.md)<br>[docs/physics/roadmap.md](docs/physics/roadmap.md) |
| Add format importers to translate real data into runtime structures. | [engine/io/importers/README.md](engine/io/include/engine/io/importers/README.md) |
| Expand the preset matrix to cover Clang and macOS toolchains once dependencies are in place. | [scripts/build/README.md](scripts/build/README.md) |
| Add scenario-driven examples and profiling to exercise the implementation. | [engine/animation/src/README.md](engine/animation/src/README.md)<br>[engine/assets/src/README.md](engine/assets/src/README.md)<br>[engine/compute/src/README.md](engine/compute/src/README.md)<br>[engine/core/ecs/README.md](engine/core/include/engine/core/ecs/README.md)<br>[engine/core/src/README.md](engine/core/src/README.md)<br>[engine/geometry/src/README.md](engine/geometry/src/README.md)<br>[engine/geometry/src/shapes/README.md](engine/geometry/src/shapes/README.md)<br>[engine/io/src/README.md](engine/io/src/README.md)<br>[engine/physics/src/README.md](engine/physics/src/README.md)<br>[engine/platform/src/README.md](engine/platform/src/README.md)<br>[engine/rendering/src/README.md](engine/rendering/src/README.md)<br>[engine/runtime/src/README.md](engine/runtime/src/README.md)<br>[engine/scene/src/README.md](engine/scene/src/README.md) |
| Author representative shader assets once the rendering pipeline is defined. | [engine/assets/shaders/README.md](engine/assets/shaders/README.md)<br>[engine/rendering/materials/shaders/README.md](engine/rendering/materials/shaders/README.md)<br>[engine/rendering/materials/shaders/common/README.md](engine/rendering/materials/shaders/common/README.md)<br>[engine/rendering/materials/shaders/glsl/README.md](engine/rendering/materials/shaders/glsl/README.md)<br>[engine/rendering/materials/shaders/hlsl/README.md](engine/rendering/materials/shaders/hlsl/README.md) |
| Bind platform abstractions to OS windowing, input, and filesystem APIs. | [engine/platform/README.md](engine/platform/README.md) |
| Broaden math primitives and numerics beyond the current minimal set. | [engine/math/README.md](engine/math/README.md) |
| Build content pipeline automation that feeds assets into the engine. | [engine/tools/pipelines/README.md](engine/tools/pipelines/README.md) |
| Design scene graph traversal primitives and dependency evaluation rules. | [engine/scene/graph/README.md](engine/scene/include/engine/scene/graph/README.md) |
| Capture design rationales and outcomes for upcoming milestones. | [docs/design/README.md](docs/design/README.md) |
| Provide concrete command encoders for the frame graph scheduler. | [engine/rendering/pipeline/README.md](engine/rendering/pipeline/README.md) |
| Execute the rendering roadmap: enrich resource descriptors, stand up schedulers, and integrate Vulkan/DX12 backends. | [engine/rendering/README.md](engine/rendering/README.md) |
| Capture shared automation entry points for developers and CI environments. | [scripts/README.md](scripts/README.md) |
| Create exporters to persist runtime assets back to disk. | [engine/io/exporters/README.md](engine/io/include/engine/io/exporters/README.md) |
| Curate representative scenarios across unit, integration, and performance suites. | [engine/tests/README.md](engine/tests/README.md) |
| Define component schemas and authoring utilities for the runtime. | [engine/scene/components/README.md](engine/scene/include/engine/scene/components/README.md) |
| Design asset pipelines that feed runtime modules beyond sample data. | [engine/assets/README.md](engine/assets/README.md) |
| Design render passes and pipelines once a resource provider is available. | [engine/rendering/passes/README.md](engine/rendering/passes/README.md) |
| Develop the editor shell and hook it into the runtime subsystems. | [engine/tools/editor/README.md](engine/tools/editor/README.md) |
| Document subsystem APIs and keep diagrams in sync with code changes. | [docs/api/README.md](docs/api/README.md) |
| Document the public headers once the API stabilises. | [engine/animation/include/engine/animation/README.md](engine/animation/include/engine/animation/README.md)<br>[engine/assets/include/engine/assets/README.md](engine/assets/include/engine/assets/README.md)<br>[engine/compute/cuda/include/engine/compute/cuda/README.md](engine/compute/cuda/include/engine/compute/cuda/README.md)<br>[engine/compute/include/engine/compute/README.md](engine/compute/include/engine/compute/README.md)<br>[engine/core/include/engine/core/README.md](engine/core/include/engine/core/README.md)<br>[engine/geometry/include/engine/geometry/README.md](engine/geometry/include/engine/geometry/README.md)<br>[engine/geometry/include/engine/geometry/shapes/README.md](engine/geometry/include/engine/geometry/shapes/README.md)<br>[engine/io/include/engine/io/README.md](engine/io/include/engine/io/README.md)<br>[engine/math/include/engine/math/README.md](engine/math/include/engine/math/README.md)<br>[engine/physics/include/engine/physics/README.md](engine/physics/include/engine/physics/README.md)<br>[engine/platform/include/engine/platform/README.md](engine/platform/include/engine/platform/README.md)<br>[engine/rendering/include/engine/rendering/README.md](engine/rendering/include/engine/rendering/README.md)<br>[engine/runtime/include/engine/runtime/README.md](engine/runtime/include/engine/runtime/README.md)<br>[engine/scene/include/engine/scene/README.md](engine/scene/include/engine/scene/README.md) |
| Drive subsystem lifecycles and state management for end-to-end scenarios. | [engine/runtime/README.md](engine/runtime/README.md)<br>[docs/design/runtime_plan.md](docs/design/runtime_plan.md) |
| Expand living design documents with current architectural decisions. | [docs/README.md](docs/README.md) |
| Expand regression coverage beyond the current smoke checks. | [engine/animation/tests/README.md](engine/animation/tests/README.md)<br>[engine/assets/tests/README.md](engine/assets/tests/README.md)<br>[engine/compute/cuda/tests/README.md](engine/compute/cuda/tests/README.md)<br>[engine/compute/tests/README.md](engine/compute/tests/README.md)<br>[engine/core/tests/README.md](engine/core/tests/README.md)<br>[engine/geometry/tests/README.md](engine/geometry/tests/README.md)<br>[engine/io/tests/README.md](engine/io/tests/README.md)<br>[engine/math/tests/README.md](engine/math/tests/README.md)<br>[engine/physics/tests/README.md](engine/physics/tests/README.md)<br>[engine/platform/tests/README.md](engine/platform/tests/README.md)<br>[engine/rendering/tests/README.md](engine/rendering/tests/README.md)<br>[engine/runtime/tests/README.md](engine/runtime/tests/README.md)<br>[engine/scene/tests/README.md](engine/scene/tests/README.md)<br>[engine/tests/integration/README.md](engine/tests/integration/README.md)<br>[engine/tests/performance/README.md](engine/tests/performance/README.md)<br>[engine/tests/unit/README.md](engine/tests/unit/README.md) |
| Extend engine core services for application control, configuration, and diagnostics. | [engine/core/README.md](engine/core/README.md) |
| Harden the loader API and expose ergonomic runtime bindings. | [python/engine3g/README.md](python/engine3g/README.md) |
| Hook materials into the rendering backend and asset pipeline. | [engine/rendering/materials/README.md](engine/rendering/materials/README.md) |
| Integrate the preset runner into hosted CI once Windows agents are available. | [scripts/ci/README.md](scripts/ci/README.md) |
| Implement GPU resource management for buffers, textures, and samplers. | [engine/rendering/resources/README.md](engine/rendering/resources/README.md)<br>[engine/rendering/resources/buffers/README.md](engine/rendering/resources/buffers/README.md)<br>[engine/rendering/resources/samplers/README.md](engine/rendering/resources/samplers/README.md) |
| Implement GPU-backed dispatch and integrate with runtime scheduling. | [engine/compute/README.md](engine/compute/README.md) |
| Integrate `InputState` with windowing backends to surface real device data. | [engine/platform/input/README.md](engine/platform/include/engine/platform/input/README.md) |
| Implement lighting models and data flows for the renderer. | [engine/rendering/lighting/README.md](engine/rendering/lighting/README.md) |
| Implement persistent cache strategies for imported assets. | [engine/io/cache/README.md](engine/io/include/engine/io/cache/README.md) |
| Expand the scene module with richer component systems, traversal helpers, and serialization coverage. | [engine/scene/README.md](engine/scene/README.md) |
| Harden scene serialization with extended component coverage and explicit versioning. | [engine/scene/serialization/README.md](engine/scene/include/engine/scene/serialization/README.md) |
| Implement scene systems that operate over ECS data during the frame. | [engine/scene/systems/README.md](engine/scene/include/engine/scene/systems/README.md) |
| Implement the actual backend integration using the target API. | [engine/rendering/backend/README.md](engine/rendering/backend/README.md)<br>[engine/rendering/backend/directx12/README.md](engine/rendering/backend/directx12/README.md)<br>[engine/rendering/backend/metal/README.md](engine/rendering/backend/metal/README.md)<br>[engine/rendering/backend/opengl/README.md](engine/rendering/backend/opengl/README.md)<br>[engine/rendering/backend/vulkan/README.md](engine/rendering/backend/vulkan/README.md) |
| Implement visibility determination algorithms to drive culling. | [engine/rendering/visibility/README.md](engine/rendering/visibility/README.md) |
| Instrument and visualise profiling data for frame and job execution. | [engine/tools/profiling/README.md](engine/tools/profiling/README.md) |
| Integrate CUDA kernels and scheduling into the dispatcher. | [engine/compute/cuda/README.md](engine/compute/cuda/README.md)<br>[engine/compute/cuda/src/README.md](engine/compute/cuda/src/README.md) |
| Extend filesystem providers with write/streaming features for tooling workflows. | [engine/platform/filesystem/README.md](engine/platform/include/engine/platform/filesystem/README.md) |
| Introduce advanced mesh processing (remeshing, parameterization, collision prep). | [engine/geometry/README.md](engine/geometry/README.md) |
| Populate this placeholder directory with its intended implementation when scoped. | [engine/README.md](engine/README.md)<br>[engine/core/application/README.md](engine/core/include/engine/core/application/README.md)<br>[engine/core/configuration/README.md](engine/core/include/engine/core/configuration/README.md)<br>[engine/core/diagnostics/README.md](engine/core/include/engine/core/diagnostics/README.md)<br>[engine/core/memory/README.md](engine/core/include/engine/core/memory/README.md)<br>[engine/core/parallel/README.md](engine/core/include/engine/core/parallel/README.md)<br>[engine/core/plugin/README.md](engine/core/include/engine/core/plugin/README.md)<br>[engine/core/runtime/README.md](engine/core/include/engine/core/runtime/README.md)<br>[engine/geometry/csg/README.md](engine/geometry/include/engine/geometry/csg/README.md)<br>[engine/geometry/decimation/README.md](engine/geometry/include/engine/geometry/decimation/README.md)<br>[engine/geometry/mesh/README.md](engine/geometry/mesh/README.md)<br>[engine/geometry/surfaces/README.md](engine/geometry/include/engine/geometry/surfaces/README.md)<br>[engine/geometry/topology/README.md](engine/geometry/include/engine/geometry/topology/README.md)<br>[engine/geometry/uv/README.md](engine/geometry/include/engine/geometry/uv/README.md)<br>[engine/geometry/volumetric/README.md](engine/geometry/include/engine/geometry/volumetric/README.md)<br>[engine/physics/collision/README.md](engine/physics/include/engine/physics/collision/README.md)<br>[engine/physics/dynamics/README.md](engine/physics/include/engine/physics/dynamics/README.md)<br>[engine/rendering/materials/textures/README.md](engine/rendering/materials/textures/README.md) |
| Prototype deformation and rigging utilities for the animation pipeline. | [engine/animation/deformation/README.md](engine/animation/include/engine/animation/deformation/README.md)<br>[engine/animation/rigging/README.md](engine/animation/include/engine/animation/rigging/README.md)<br>[engine/animation/skinning/README.md](engine/animation/include/engine/animation/skinning/README.md) |
| Provide curated sample assets demonstrating runtime features. | [engine/assets/samples/README.md](engine/assets/samples/README.md) |
| Publish the Python packaging and dependency manifest for automation helpers. | [python/README.md](python/README.md) |
| Expand Python loader tests with real libraries, diagnostics, and shared fixtures. | [python/tests/README.md](python/tests/README.md) |
| Replace the stub window factories with real OS backends. | [engine/platform/windowing/README.md](engine/platform/include/engine/platform/windowing/README.md) |
| Stabilise cross-module integration and build targets for contributors. | [README.md](README.md) |
| Stand up tooling flows for content pipelines, profiling, and editor workflows. | [engine/tools/README.md](engine/tools/README.md) |
| Execute the geometry I/O roadmap (robust detection, pluggable formats, cache/stream integration). | [engine/io/README.md](engine/io/README.md) |