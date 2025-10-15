# Glossary

| Term | Definition |
| --- | --- |
| **ADR** | Architecture Decision Record. Stored under [`docs/specs/`](specs/) and treated as binding until superseded. |
| **AI-003** | Architecture improvement plan item expanding frame-graph metadata and validation. See [`docs/ROADMAP.md`](ROADMAP.md). |
| **DC-001** | Critical design correction introducing subsystem interfaces and plugin discovery in the runtime module. |
| **Frame Graph** | The dependency graph describing rendering passes, resources, and synchronisation. Defined in [`docs/specs/ADR-0003-runtime-frame-graph.md`](specs/ADR-0003-runtime-frame-graph.md). |
| **RuntimeHost** | Entry point that advances animation, physics, geometry, and rendering submission. Documented in `docs/modules/runtime/README.md`. |
| **Spatial Index** | Acceleration structure (kd-tree, octree) maintained by `engine/geometry` to speed spatial queries. |
| **Task Record** | A Markdown file under [`docs/tasks/`](tasks/) containing goal, inputs, constraints, deliverables, and acceptance checklist for a piece of work. |

Add new terms when they become part of regular discussions or when onboarding feedback reveals ambiguity.
