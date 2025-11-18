---
id: TL-316
title: Vertex, edge, and face selection for geometry entities
status: in_progress
priority: P1
area: geometry
size: M
owner: geometry
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on:
  - "TL-315"
links:
  - "docs/modules/geometry/README.md"
  - "docs/modules/tools/README.md"
  - "docs/specs/ADR_0009_GEOMETRY_RESOURCE_PIPELINE.md"
---

# Task TL-316 — Vertex, Edge, and Face Selection for Geometry Entities

## Intent

Report and manipulate per-entity sub-selection (vertices, edges, faces, and point-cloud samples) so the editor and runtime tooling can expose fine-grained editing, inspection, and diagnostics on selected geometry.

---

## Context

**Current State:**
- Even after TL-315, the selection stack only returns entity IDs; tooling cannot tell which mesh primitives a user clicked.
- Mesh data, point clouds, and graph primitives already exist in the geometry module but lack consistent handles or metadata for selection events.
- Diagnostics (surface normals, adjacency, attribute overlays) cannot scope to user-selected primitives, limiting debugging workflows.

**Desired State:**
- Selection events include optional primitive hits (vertex indices, edge pairs, face IDs, or point IDs) when the target entity exposes geometry data.
- Surfaces, graphs, and point clouds register selection adapters so TL-311 hierarchy panels and future editing tools can list sub-selection details.
- Vertex/edge/face sets can be retrieved for marquee selections, with configurable caps to prevent unbounded growth.

**References:**
- [`docs/modules/geometry/README.md`](../../docs/modules/geometry/README.md) — mesh/point-cloud data ownership.
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — UI/panel hooks that need sub-selection payloads.
- [`docs/specs/ADR_0009_GEOMETRY_RESOURCE_PIPELINE.md`](../../docs/specs/ADR_0009_GEOMETRY_RESOURCE_PIPELINE.md) — guarantees for geometry handles and buffer lifetimes.

---

## Design / Plan

### Constraints

- Must not mutate source mesh buffers; selection adapters operate on handles/IDs only.
- Support multiple primitive types simultaneously (e.g., mesh with both face and vertex hits) without ambiguous ordering.
- Keep adapters lazy; they should compute barycentric coordinates or adjacency only when needed.
- Respect TL-315 ordering semantics so sub-selections align with entity order.

### API / Data Sketch

```cpp
struct PrimitiveHit {
  EntityId entity;
  SelectionPrimitive type; // vertex, edge, face, point
  uint32_t index0;
  uint32_t index1; // optional (edge vertex B)
  float barycentric[3];
};

class PrimitiveSelectionAdapter {
 public:
  virtual std::optional<PrimitiveHit> pick_primitive(
      const SelectionHit& hit,
      const GeometryComponent& geometry,
      const CursorRay& ray) = 0;
  virtual PrimitiveSet marquee_select(const Frustum& world_region) = 0;
};
```

### Edge Cases & Failure Modes

- **Non-manifold meshes:** ensure barycentric interpolation detects degenerate faces; fall back to vertex selection.
- **Sparse point clouds:** sample density might make hits ambiguous; provide configurable radius thresholds.
- **Graph primitives without geometry buffers:** allow adapters that work purely from logical graph structures.
- **Large marquee selections:** stream indices (chunked vectors) to avoid memory blow-ups.

### Test Plan

- **Unit Tests:**
  - Primitive hit computation given known triangle and cursor ray.
  - Edge selection ensures consistent ordering of vertex pair indices.
  - Point-cloud radius tests verifying inclusion/exclusion thresholds.
- **Integration Tests:**
  - Selection engine integration verifying primitive metadata flows into tooling panels.
  - Geometry viewer sample verifying highlighted vertices/edges/faces follow clicks.
- **Regression Tests:**
  - Protect against stale geometry handles after buffer streaming; ensure adapters rebind correctly.
  - Verify marquee selection respects max primitive count and gracefully truncates.

### Tool Integration

**Diagnostic UI:**
- [ ] Extend hierarchy/performance panels to show primitive counts for selected entities.
- [ ] Provide overlay rendering (e.g., highlight vertices) for debugging.

**Benchmark Automation:**
- [ ] Add optional micro-benchmarks measuring primitive hit throughput on representative meshes.

---

## Steps

1. [x] Align with TL-315 selection payload schema for primitive metadata.
2. [ ] Implement primitive adapters for triangle meshes, line graphs, and point clouds.
3. [x] Add marquee helpers that return capped primitive sets with streaming iterators.
4. [ ] Surface primitive info through tooling APIs and diagnostics overlays.
5. [ ] Cover adapters with targeted geometry unit tests and geometry_viewer integration tests.
6. [x] Document workflows in geometry + tools READMEs and update roadmap/backlog status.

---

## Evidence

### Test Results

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target engine_scene_tests
ctest --preset linux-gcc-debug -R engine_scene_tests --output-on-failure
pytest python/tests scripts/tests
```

- CMake configure: see linux-gcc-debug preset configure log. 【343f81†L1-L20】
- Scene tests build succeeded (includes new primitive selection unit tests). 【588ce4†L1-L2】
- `engine_scene_tests` ctest target passes. 【746e81†L1-L8】
- Python + maintenance test suites: 288 passed, 3 skipped. 【7ab1ea†L1-L19】
