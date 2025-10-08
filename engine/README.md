# Engine Source Tree

The `engine/` subtree contains the core C++ implementation of the engine. Each subsystem is isolated in its own
directory with a companion README that describes its scope and internal layout.

## Module Purpose

- `animation/` – Rigging, deformation, and runtime playback experiments.
- `assets/` – Asset pipelines, sample content, and shader packaging.
- `compute/` – Heterogeneous compute backends (CUDA and generic dispatch infrastructure).
- `core/` – Foundational runtime services, configuration, diagnostics, and ECS primitives.
- `geometry/` – Mesh processing, topology utilities, surface/volume representations, and discrete differential geometry
  algorithms.
- `io/` – Import/export modules and caching layers for content.
- `math/` – Shared mathematical utilities (vectors, matrices, quaternions, transforms, camera helpers).
- `physics/` – Collision detection and dynamics scaffolding.
- `platform/` – Abstractions for windowing, input, and file systems.
- `rendering/` – Rendering pipeline layers, material systems, and backend integrations.
- `runtime/` – Application glue that stitches systems together and exposes a shared-library discovery surface.
- `scene/` – Scene graph, component registration, and serialization built around EnTT.
- `tests/` – Unit, integration, and performance test suites organised by subsystem (math numerics enabled today).
- `tools/` – Developer tooling (profilers, content pipelines, and editor stubs).

## Dependencies

- C++20 toolchain matching the root README prerequisites.
- CMake 3.26+ for generating project files.
- GoogleTest (vendored under `third_party/googletest`) for unit tests.
- Optional GPU SDKs/driver packages depending on the rendering/compute backends you enable (Vulkan, CUDA, DirectX 12,
  OpenGL).

## Setup and Build

From the repository root:

```bash
cmake -S . -B build -G Ninja
cmake --build build --target engine_runtime
```

Subsystem targets follow the `engine_<subsystem>` naming convention so they can be located dynamically by the runtime
loader and by the Python tooling (`python/engine3g/loader.py`).

## Test Commands

- `ctest --test-dir build` – Executes the GoogleTest suites located under `engine/tests/`.
- `pytest` – Complements the native tests by exercising the Python loader against the built shared libraries.

_Last updated: 2025-02-14_
