# Specifications & Decision Records

This directory houses ADRs, RFPs, and deep dives. They are the binding source of truth for architectural choices until explicitly superseded.

## Index

- [`ADR_0003_RUNTIME_FRAME_GRAPH.md`](ADR_0003_RUNTIME_FRAME_GRAPH.md) – scheduler requirements, metadata model, and backend integration strategy.
- [`ADR_0005_GEOMETRY_IO_ROUNDTRIP.md`](ADR_0005_GEOMETRY_IO_ROUNDTRIP.md) – geometry/IO ownership, file formats, and validation pipeline.
- [`ADR_0006_ANIMATION_DEFORMATION.md`](ADR_0006_ANIMATION_DEFORMATION.md) – runtime linear blend skinning pipeline and rig binding requirements.
- [`AN_240_STATE_MACHINE_AUTHORING.md`](AN_240_STATE_MACHINE_AUTHORING.md) – specification for animation state-machine authoring, serialization, and tooling integration.
- [`ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) – runtime loop configurability, presentation backends, and reusable ImGui panel registry.

When creating a new record:

1. Copy the structure from `ADR-0003` (Title, Status, Context, Decision, Consequences).
2. Reference relevant roadmap items (`DC-`, `AI-`, `RT-`).
3. Link impacted modules and update their READMEs.
4. Add the file to this index and cross-link from `docs/ARCHITECTURE.md` if the decision introduces new invariants.
