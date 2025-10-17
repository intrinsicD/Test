# Specifications & Decision Records

This directory houses ADRs, RFPs, and deep dives. They are the binding source of truth for architectural choices until explicitly superseded.

## Index

- [`ADR-0003-runtime-frame-graph.md`](ADR-0003-runtime-frame-graph.md) – scheduler requirements, metadata model, and backend integration strategy.
- [`ADR-0005-geometry-io-roundtrip.md`](ADR-0005-geometry-io-roundtrip.md) – geometry/IO ownership, file formats, and validation pipeline.
- [`ADR-0006-animation-deformation.md`](ADR-0006-animation-deformation.md) – runtime linear blend skinning pipeline and rig binding requirements.

When creating a new record:

1. Copy the structure from `ADR-0003` (Title, Status, Context, Decision, Consequences).
2. Reference relevant roadmap items (`DC-`, `AI-`, `RT-`).
3. Link impacted modules and update their READMEs.
4. Add the file to this index and cross-link from `docs/architecture.md` if the decision introduces new invariants.
