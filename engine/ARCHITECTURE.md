# Engine Module Architecture

## Dependency Layers (Bottom to Top)

### Layer 0: Foundation (No Engine Dependencies)
- **math** – Vector, matrix, quaternion, and transform primitives.
  - Dependencies: C++ standard library only.
  - Consumed by: Every other module.

### Layer 1: Core Data & Services
- **animation** – Skeletal clip sampling, blend-tree evaluation, controller utilities.
  - Depends on: `engine::math`.
- **compute** – Kernel dispatcher and execution profiling utilities.
  - Depends on: `engine::math`.
- **geometry** – Shapes, meshes, spatial queries, and property registries.
  - Depends on: `engine::math`.
- **platform** – Filesystem, windowing, and input abstractions with mock backends.
  - Depends on: `engine::math`.

### Layer 2: Domain Facades (May depend on Layers 0–1)
- **assets** – Runtime caches for geometry, textures, and shader metadata with hot-reload hooks.
  - Depends on: `engine::geometry`, `engine::io`.
- **core** – Shared identifiers and module discovery helpers that feed the runtime.
  - Depends on: `engine::math`.
- **io** – Geometry/animation import-export helpers and format detection.
  - Depends on: `engine::geometry`.
- **physics** – Rigid body world, force accumulation, and collider management.
  - Depends on: `engine::math`, `engine::geometry`.
- **scene** – EnTT-based entity façade, hierarchy systems, and serialization helpers.
  - Depends on: `engine::math`, `engine::animation`.

### Layer 3: Orchestration & Presentation (May depend on Layers 0–2)
- **rendering** – CPU-side frame graph, forward pipeline prototype, and command encoder contracts.
  - Depends on: `engine::math`, `engine::geometry` (and optionally `engine::animation`, `engine::physics`).
- **runtime** – `RuntimeHost` that coordinates animation, physics, geometry, and compute subsystems.
  - Depends on: `engine::animation`, `engine::assets`, `engine::compute`, `engine::geometry`, `engine::physics`, `engine::scene`.
- **tools** – Future home for editor shells, profilers, and automation (currently scaffolding only).
  - Depends on: Modules it integrates; keep adapters thin to avoid new upward edges.

## Dependency Rules

1. **Downward dependencies only**: Higher layers depend on lower ones, never upward.
2. **Avoid lateral coupling**: Modules in the same layer do not depend on one another without an explicit architectural exception.
3. **Promote shared utilities**: If two modules in the same layer need a capability, promote it to a lower layer (usually Layer 0 or 1).

## Cross-Cutting Concerns

### Shared Types
Common linear algebra and transform types live in `engine::math`:
- `vec3`, `vec4`, `quat`, `mat4`
- `Transform` (position, rotation, scale)
- `Color`

### Error Handling
Each module owns its error domain but routes diagnostic output through the shared logging utilities defined in `engine::platform`.

### Threading
Modules must be thread-safe for read access. Write access requires external synchronisation, typically orchestrated by the runtime layer.

### Build Artifacts
Every module exposes a CMake target `engine::<module>` and uses the `ENGINE_<MODULE>_API` visibility macro. Static and shared builds honour the same dependency layering rules.

## Adding or Modifying Dependencies

1. Confirm that the new dependency obeys the layering rules above.
2. Update this document and the relevant module READMEs to reflect the change.
3. Add or adjust include dependency checks to ensure the layering cannot be violated during builds.

## Preventing Circular Dependencies

- Keep interfaces narrow and pass data via value types or well-defined service abstractions.
- Share types through the lowest applicable layer (`engine::math` or a new foundational module) instead of peer-to-peer headers.
- Use adapter modules when integration requires translating between domains rather than creating cyclic includes.
