# Engine Source Tree

The `engine/` subtree contains the core C++ implementation of the engine. Each subsystem is isolated in its own directory with a matching README that summarises its state.

## Subsystems
- `animation/` – Rigging, deformation, and animation runtime experiments.
- `assets/` – Asset pipelines, sample content, and shader packaging.
- `compute/` – Heterogeneous compute backends (CUDA and generic dispatch infrastructure).
- `core/` – Foundational runtime services, configuration, diagnostics, and ECS primitives.
- `geometry/` – Mesh processing, topology utilities, surface/volume representations, and discrete differential
  geometry algorithms.
- `io/` – Import/export modules and caching layers for content.
- `math/` – Shared mathematical utilities (vectors, matrices, quaternions, transforms, camera helpers).
- `physics/` – Collision detection and dynamics scaffolding.
- `platform/` – Abstractions for windowing, input, and file systems.
- `rendering/` – Rendering pipeline layers, material systems, and backend integrations.
- `runtime/` – Application glue that stitches systems together and exposes a shared-library discovery surface.
- `scene/` – Scene graph, component registration, and serialization built around EnTT.
- `tests/` – Unit, integration, and performance test suites organised by subsystem (math numerics enabled today).
- `tools/` – Developer tooling (profilers, content pipelines, and editor stubs).

A top-level `CMakeLists.txt` exposes each subsystem to the global build configuration. Build targets follow the
`engine_<subsystem>` naming convention so they can be located dynamically by the runtime loader.

_Last updated: 2025-02-14_
