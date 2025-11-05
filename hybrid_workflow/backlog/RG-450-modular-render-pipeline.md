---
id: RG-450
title: Modular render pipeline planner
status: new
priority: P1
area: rendering
size: L
owner: rendering-lead
gates: [tests, perf, docs, safety, release]
relates_to: [bundle:A]
blocked_on: ["T-0120", "T-0119", "RT-410"]
links: ["docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md", "docs/modules/rendering/README.md"]
---

# Task RG-450 — Modular Render Pipeline Planner

## Intent

Deliver a hot-swappable modular render pipeline planner so the renderer can compose frame graphs from plugin-provided nodes and execute them across async queues with automatic resource lifetime management.

---

## Context

**Current State:**
- Frame execution relies on hard-wired pass orderings that assume a fixed G-Buffer → lighting → post-processing chain.
- Resource reuse requires manual aliasing decisions and prevents rapid iteration or hot-reload of render passes.
- Plugin nodes cannot self-describe their resource contracts, limiting experimentation with hybrid raster + ray tracing or vector-field pipelines.

**Desired State:**
- Nodes advertise their created, read, and written resources plus preferred queues, enabling automated scheduling.
- A registry collects built-in and plugin node factories, supporting hot-reload and profile-specific graph assembly.
- The planner resolves dependencies, assigns transient resources, injects barriers, and emits an executable graph per frame.
- Runtime execution supports multiple pipeline presets (PBR deferred, hybrid RT, vector-field, VR, headless) without recompilation.

**References:**
- ADR-0003 Runtime Frame Graph interface contracts.
- Rendering module README for existing frame graph expectations.
- Plugin architecture guidelines in `docs/design/PLUGIN_ARCHITECTURE.md`.
- Resource management design in `docs/design/RESOURCE_MANAGEMENT.md`.

---

## Design / Plan

### Constraints

- Follow `hybrid_workflow/CONTRIBUTING.md` coding standards and task lifecycle checkpoints.
- Preserve determinism in graph planning and explicit barrier derivation per ADR-0003.
- Expose planner state to telemetry without regressing capture overhead beyond the ±2% budget.
- Keep node reflection purely declarative to enable hot-reload safety guarantees from the plugin architecture design.
- Coordinate resource allocation strategy with the GPU resource provider and command encoder milestones (T-0120, T-0119).

### API / Data Sketch

```cpp
struct ResourceDesc {
  enum class Kind { Texture, Buffer, TLAS, AccelScratch, External };
  Kind kind;
  VkFormat format{};
  uint32_t width{};
  uint32_t height{};
  uint32_t layers{};
  uint32_t mips{};
  std::string name;
  bool transient{true};
};

struct ResourceUse {
  std::string name;
  VkPipelineStageFlags2 stage;
  VkAccessFlags2 access;
  VkImageLayout layout;
};

struct NodeDescriptor {
  std::string id;
  std::vector<ResourceDesc> creates;
  std::vector<ResourceUse> reads;
  std::vector<ResourceUse> writes;
  std::vector<std::string> tags;
  int preferred_queue = 0;
};

class INode {
public:
  virtual ~INode() = default;
  virtual const NodeDescriptor& Reflect() const = 0;
  virtual void Compile(NodeContext& ctx) = 0;
  virtual void Execute(NodeContext& ctx) = 0;
};
```

### Edge Cases & Failure Modes

- **Node reflection mismatch:** Reject graphs when multiple nodes claim write access to the same resource; emit diagnostics referencing offending descriptors.
- **Hot-reload churn:** Ensure transient allocator tears down and rebuilds safely when plugin DLLs update; preserve persistent history resources via compatibility checks.
- **Queue conflicts:** Detect when compute-only nodes are scheduled on graphics queues due to missing capabilities; fall back gracefully while logging.
- **External attachment changes:** Validate swapchain or XR view descriptors on plan rebuilds; refuse execution if formats diverge unexpectedly.
- **Resource exhaustion:** Guard transient allocator growth and surface telemetry counters when aliasing fails.

### Test Plan

- **Unit Tests:**
  - Descriptor validation rejects duplicate writers and missing producers.
  - Planner topological sort respects declared dependencies and tags.
  - Transient allocator reuses resources across compatible lifetimes.
  - History persistence retains named resources across plan rebuilds.
  - Hot-reload rebuild triggers correct re-planning with updated node reflections.

- **Integration Tests:**
  - Execute sample PBR and hybrid RT profiles through the planner with mock backends.
  - Validate asynchronous queue partitioning once compute-only nodes are registered.
  - Demonstrate plugin hot-swap by loading/unloading a DLL node and observing plan rebuild without restarting.
  - Run vector-field profile to ensure non-PBR tags compose correctly.

- **Performance:**
  - Capture planner scheduling time and allocator churn versus baseline hard-wired pipeline (<2% regression allowed).
  - Measure GPU queue overlap efficiency when async compute is enabled.

- **Regression Tests:**
  - Continuous validation in PM-510 weekly demos with telemetry snapshots.
  - Snapshot DOT exports of the render graph for documentation diffs.

---

## Steps

1. [ ] Draft planner architecture notes and confirm alignment with ADR-0003 and plugin constraints.
2. [ ] Implement `NodeDescriptor`, `ResourceDesc`, `ResourceUse`, and `INode` interfaces.
3. [ ] Build registry/factory system with hot-reload hooks and plugin registration surface.
4. [ ] Implement initial planner: resolve dependencies, allocate transients, emit single-queue schedule.
5. [ ] Add runtime execution path driving per-frame graph execution with resource state transitions.
6. [ ] Integrate transient allocator pooling textures/buffers keyed by descriptor compatibility.
7. [ ] Enable queue partitioning and async compute overlap when descriptors allow.
8. [ ] Add persistent history resource compatibility checks and telemetry counters.
9. [ ] Produce DOT/telemetry exports and integration demos recorded under PM-510.
10. [ ] Update documentation (rendering module README, ROADMAP) and finalize task evidence.

---

## Evidence

### Test Results

```bash
# cmake --preset linux-gcc-debug
# cmake --build --preset linux-gcc-debug
# ctest --preset linux-gcc-debug
# pytest python/tests scripts/tests
# python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: [pending]
- Integration tests: [pending]
- Documentation validation: [pending]

### Performance (if applicable)

**Benchmark:** Planner scheduling overhead vs hard-wired pipeline
- Before: [baseline metric]
- After: [new metric]
- Delta: [pending]

**Artifacts:**
- Telemetry captures: `telemetry/render_graph/*.json`
- Planner DOT exports: `docs/modules/rendering/graphs/*.dot`

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] | QA/Test | Planner + runtime validation |
| perf | [ ] | Performance | Benchmark deltas |
| docs | [ ] | Docs/DevRel | Updated READMEs + roadmap |
| safety | [ ] | Safety | Plugin hot-reload checklist |
| release | [ ] | Release Mgr | Packaging notes |

### Updated Files

- `engine/rendering/include/engine/rendering/framegraph/*`
- `engine/rendering/src/framegraph/*`
- `engine/rendering/tests/framegraph/*`
- `docs/modules/rendering/README.md`
- `docs/modules/rendering/render_graph.md`
- `docs/NAVIGATION.md`
- `hybrid_workflow/backlog/RG-450-modular-render-pipeline.md`

---

## Completion Checklist (Definition of Done)

- [ ] Planner composes pipelines from descriptor-driven nodes with transient allocator support.
- [ ] Async queue partitioning enabled with telemetry coverage.
- [ ] Plugin hot-reload validated with automated task coverage.
- [ ] Documentation (module README, roadmap, DOT exports) updated.
- [ ] Quality gates signed off in Evidence section.

---

## Notes

- Coordinate with GPU resource provider (T-0120) and command encoder integration (T-0119) to ensure resource handles map cleanly into backend command buffers.
- Schedule intermediate demo with PM-510 to broadcast hot-reload and profile toggling capabilities once initial planner lands.
