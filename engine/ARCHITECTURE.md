# Engine Module Architecture

## Dependency Layers (Bottom to Top)

### Layer 0: Foundation (No Engine Dependencies)
- **math**: Vector, matrix, and quaternion math primitives.
  - Dependencies: C++ standard library only.
  - Consumed by: Every other module.

### Layer 1: Core Utilities
- **platform**: OS abstraction, window management, input services.
  - Depends on: `engine::math`.
- **memory**: Allocators and memory pools that back higher-level systems.
  - Depends on: `engine::platform`.

### Layer 2: Domain Modules (May depend on Layers 0–1)
- **geometry**: Shapes, meshes, and spatial queries.
  - Depends on: `engine::math`.
  - Exports: Collision primitives for physics and bounds for rendering.
- **animation**: Skeletal animation, blend trees, and deformation utilities.
  - Depends on: `engine::math`.

### Layer 3: Simulation (May depend on Layers 0–2)
- **physics**: Rigid body dynamics, collision detection, and world management.
  - Depends on: `engine::math`, `engine::geometry`, `engine::platform`.

### Layer 4: Presentation (May depend on Layers 0–3)
- **rendering**: Graphics API abstraction, frame graph orchestration, scene graph.
  - Depends on: `engine::math`, `engine::geometry`, `engine::platform` (and optionally `engine::animation`, `engine::physics`).

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
