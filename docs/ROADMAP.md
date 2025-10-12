# Central Roadmap

This roadmap aggregates the actionable plans defined by each engine module and highlights cross-cutting initiatives. Consult the per-module documents for detailed task descriptions.

## Module Roadmaps

- [Animation](modules/animation/ROADMAP.md)
- [Assets](modules/assets/ROADMAP.md)
- [Compute](modules/compute/ROADMAP.md)
- [Core](modules/core/ROADMAP.md)
- [Geometry](modules/geometry/ROADMAP.md)
- [IO](modules/io/ROADMAP.md)
- [Math](modules/math/ROADMAP.md)
- [Physics](modules/physics/ROADMAP.md)
- [Platform](modules/platform/ROADMAP.md)
- [Rendering](modules/rendering/ROADMAP.md)
- [Runtime](modules/runtime/ROADMAP.md)
- [Scene](modules/scene/ROADMAP.md)
- [Tools](modules/tools/ROADMAP.md)

## Cross-Cutting Focus Areas

- **Data Contracts:** Geometry, animation, and physics must continue aligning mesh/rig/collider representations to guarantee interoperability. Progress is tracked in the respective module roadmaps.
- **Scheduling Infrastructure:** Compute and rendering share work on job graphs and GPU scheduling. Coordinate milestones to avoid conflicting abstractions.
- **Asset Flow:** IO and assets collaborate on importer accuracy and cache invalidation so runtime/tooling can rely on deterministic content delivery.
- **Diagnostics:** Platform, rendering, runtime, and tools should converge on logging and telemetry schemas for unified observability.

For longer-range planning, refer to the [global roadmap overview](global_roadmap.md), which captures milestone sequencing across releases.
