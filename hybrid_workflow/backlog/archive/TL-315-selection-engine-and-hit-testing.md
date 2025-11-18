---
id: TL-315
title: Selection engine and hit-testing pipeline
status: done
priority: P1
area: tools
size: L
owner: tools-lead
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on: []
links:
  - "docs/modules/tools/README.md"
  - "docs/modules/runtime/README.md"
  - "docs/modules/rendering/README.md"
  - "docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md"
---

# Task TL-315 — Selection Engine and Hit-Testing Pipeline

## Intent

Deliver an extensible selection system so runtime/editor tooling can select one or more scene entities reliably, with an ordered selection stack and pluggable hit-testing strategies that scale from brute-force queries to spatial indices or color-ID rendering.

---

## Context

**Current State:**
- TL-310 re-enabled the editor harness but it still lacks a unified selection service; panels have no way to coordinate hit tests across the scene graph.
- Scene entity picking currently relies on ad-hoc ImGui callbacks or external scripts, leading to inconsistent UX and no multi-selection ordering.
- Selection algorithms are undefined, so runtime builds cannot fall back to brute-force bounding-box tests when spatial data is missing.

**Desired State:**
- The engine exposes a `SelectionEngine` interface that stores ordered selections, exposes change notifications, and accepts swappable `SelectionStrategy` implementations (scene graph query, BVH, GPU color picking, etc.).
- Tooling modules can query the selection stack regardless of how hit-testing occurred, enabling panels such as TL-311 (hierarchy) and TL-312 (performance) to stay in sync.
- Fallback logic ensures we can still compute selection via cursor→world transforms and bounding-box intersections when scene data lacks spatial acceleration structures.

**References:**
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — editor and panel integration guarantees.
- [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) — runtime presentation adapters and scene graph ownership.
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) — renderer responsibilities for color picking.
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) — synchronization contract between runtime and tooling subsystems.

---

## Design / Plan

### Constraints

- Preserve determinism: selection order must match click chronology even when duplicate IDs arrive.
- Avoid blocking the render thread; hit-testing should enqueue jobs or reuse spatial indices generated during culling.
- Keep strategies swappable without recompiling panels; prefer interfaces or strategy registry in the tools module.
- Fallback path must degrade gracefully to brute-force bounding-box iteration when spatial indices are unavailable.
- Color picking path must reuse existing offscreen render targets and respect renderer resource budgets.

### API / Data Sketch

```cpp
struct SelectionEvent {
  EntityId id;
  SelectionHit hit;
  SelectionSource source; // cursor, marquee, script
};

class SelectionStrategy {
 public:
  virtual SelectionHit try_pick(const Ray& cursor_ray,
                                SelectionContext& ctx) = 0;
};

class SelectionEngine {
 public:
  void register_strategy(std::unique_ptr<SelectionStrategy> strategy,
                         SelectionPriority priority);
  void push_selection(const SelectionEvent& event);
  std::span<const SelectionEvent> ordered_selection() const;
  void clear_selection();
};
```

### Edge Cases & Failure Modes

- **Missing world transforms:** fallback to object-local bounds via stored scale/rotation; flag entities with invalid transforms.
- **Multiple hits:** define deterministic tie-breaking (nearest depth, highest priority strategy, entity priority tag).
- **Color render fallback:** handle cases where the ID render target is stale or not yet produced; re-render lazily.
- **Mass selection:** when marquee or brush selects thousands of entities, cap stack size or stream updates to avoid blocking the UI thread.

### Test Plan

- **Unit Tests:**
  - Verify ordered selection stack preserves insertion order and clearing semantics.
  - Mock strategies to ensure priority ordering and fallback invocation.
  - Validate bounding-box fallback identifies hits given cursor rays and transforms.
- **Integration Tests:**
  - Editor harness test injecting synthetic clicks through the panel registry to assert selection notifications fire.
  - Runtime test that color-ID rendering returns consistent entity IDs under multi-sample pipelines.
- **Regression Tests:**
  - Ensure repeated clicks on the same entity update timestamps without duplicating entries when deduplication is enabled.
  - Guard against selection stack overflow with large marquee operations.

### Tool Integration

**Profiling:**
- [ ] Instrument strategy evaluation with `PROFILE_SCOPE("SelectionStrategy::try_pick")` to track hotspots.

**Diagnostic UI:**
- [ ] Provide ImGui overlay to visualize cursor rays and bounding volumes for debugging fallback logic.

**Benchmark Automation:**
- [ ] Optional micro-benchmark comparing BVH queries vs. brute-force fallback to justify future perf gates.

---

## Steps

1. [x] Audited the scene graph traversal, runtime panel bridge hooks, and docs to capture constraints for deterministic
   selection ordering.
2. [x] Implemented the reusable `SelectionEngine` plus listener API, priority-ordered strategy registry, and evidence-backed
   unit tests.
3. [x] Added the `BoundingBoxSelectionStrategy` fallback that derives world-space AABBs from transforms and reuses the existing
   ray–box intersection helpers.
4. [x] Extended the scene hierarchy panel + runtime panel bridge to bind selection engines, propagate editor clicks, and consume
   shared selection notifications.
5. [x] Updated the tools module docs to describe the new selection architecture and recorded validation evidence.
6. [x] Relocated the reusable selection engine + fallback strategy into the scene module so runtime systems and editor tooling
   depend on a neutral subsystem instead of `engine_tools`.

---

## Evidence

- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug --target test_tools_module`
- `ctest --preset linux-gcc-debug -R test_tools_module`
- `ctest --preset linux-gcc-debug -R engine_scene_tests`
- `python scripts/validate_docs.py`
