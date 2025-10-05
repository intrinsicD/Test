# Engine Source Tree

The `engine/` subtree contains the core C++ implementation of the engine. Each subsystem is isolated in its own directory with a matching README that summarises its state.

## Subsystems
- `animation/` – Rigging, deformation, and animation runtime experiments.
- `assets/` – Asset pipelines, sample content, and shader packaging.
- `compute/` – Heterogeneous compute backends (CUDA and generic dispatch infrastructure).
- `core/` – Foundational runtime services, configuration, diagnostics, and ECS primitives.
- `geometry/` – Mesh processing, topology utilities, and geometric algorithms.
- `io/` – Import/export modules and caching layers for content.
- `math/` – Shared mathematical utilities.
- `physics/` – Collision detection and dynamics scaffolding.
- `platform/` – Abstractions for windowing, input, and file systems.
- `rendering/` – Rendering pipeline layers, material systems, and backend integrations.
- `runtime/` – Application glue that stitches systems together.
- `scene/` – Scene graph, component registration, and serialization.
- `tests/` – Unit, integration, and performance test suites organised by subsystem.
- `tools/` – Developer tooling (profilers, content pipelines, and editor stubs).

A top-level `CMakeLists.txt` exposes each subsystem to the global build configuration.

_Last updated: 2025-10-05_
