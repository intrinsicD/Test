# ADR-0005: Geometry & IO Roundtrip Fidelity

- **Status:** Draft
- **Drivers:** `RT-006`, `AI-001`
- **Authors:** Geometry & IO Working Group
- **Last Updated:** 2025-02-17

## Context
Geometry and IO modules currently support ASCII mesh/point-cloud/graph formats. Spatial indices (kd-tree, octree) depend on consistent bounds and centroid updates. Bugs have surfaced around fat-object support and degenerate primitives when round-tripping assets through IO.

## Decision
1. **Single Source of Truth:** Geometry owns canonical structures (`SurfaceMesh`, `PointCloud`, `Graph`). IO adapters convert to/from these types only.
2. **Validation Pipeline:** Importers normalise winding, remove duplicate vertices, and compute bounds/centroids. Exporters verify invariants before serialising.
3. **Spatial Synchronisation:** Any geometry mutation invalidates cached acceleration structures; rebuilds occur lazily but deterministically before queries.
4. **Checksum Coverage:** Round-trip operations compute deterministic checksums recorded alongside fixtures to detect drift.

## Consequences
- All import/export helpers must call geometry validators. Skipping them is a bug.
- Test fixtures in `engine/geometry/tests/` must include checksum baselines for meshes, point clouds, and graphs.
- Binary format support requires an ADR update with explicit invariants before implementation begins.

## Roadmap Alignment
- Supports `RT-006` (IO hardening) by ensuring deterministic round-tripping.
- Enables `AI-001` by guaranteeing resource handles remain valid after IO/geometry conversions.

## Follow-Up Work
- Implement the acceptance criteria in [`docs/tasks/T-0112-geometry-io-roundtrip-hardening.md`](../tasks/T-0112-geometry-io-roundtrip-hardening.md).
- Update `docs/modules/geometry/README.md` and `docs/modules/io/README.md` once the pipeline is live.
