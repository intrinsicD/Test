# T-0112: Geometry/IO Roundtrip Hardening

## Goal
Guarantee lossless round-tripping for meshes, point clouds, and graphs by aligning geometry invariants with IO validators.

## Inputs
- Code: `engine/geometry/`, `engine/io/`, `engine/geometry/tests/`
- Specs: [`docs/specs/ADR-0005-geometry-io-roundtrip.md`](../specs/ADR-0005-geometry-io-roundtrip.md)
- Roadmap: [`RT-006`](../specs/ADR-0005-geometry-io-roundtrip.md#roadmap-alignment)

## Constraints
- No additional allocations in kd-tree/octree hot loops.
- Support ASCII formats today; document binary roadmap in the spec.
- Maintain compatibility with existing regression fixtures under `engine/geometry/tests/data/`.

## Deliverables
- Deterministic import/export helpers with checksum validation.
- Extended regression tests covering degenerate faces and sparse graphs.
- Updated documentation in module READMEs and specs.

## Checklist
- [ ] Added tests exercise fat objects and point-cloud edge cases.
- [ ] Benchmarks recorded in this file.
- [ ] Dependency notes updated in `docs/modules/io/README.md` and `docs/modules/geometry/README.md`.
