---
id: RG-450-A
title: Node descriptor API design
status: ready
priority: P1
area: rendering
size: S
owner: rendering-lead
gates: [docs]
relates_to: [bundle:A, RG-450]
blocked_on: []
links: ["docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md", "hybrid_workflow/backlog/RG-450-modular-render-pipeline.md"]
---

# Task RG-450-A — Node Descriptor API Design

## Intent

Design the node descriptor API so plugin-provided render passes can self-describe their resource contracts and queue preferences.

---

## Context

**Current State:**
- RG-450 parent task defines modular render pipeline planner goals
- T-0120 and T-0119 (GPU resource provider and command encoder) are complete
- Node reflection API needs design before registry implementation

**Desired State:**
- NodeDescriptor, ResourceDesc, ResourceUse structures defined
- INode interface documented with reflection contracts
- Clear guidance for plugin authors on implementing render nodes

**References:**
- [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](../../docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md)
- [`hybrid_workflow/backlog/RG-450-modular-render-pipeline.md`](RG-450-modular-render-pipeline.md)

---

## Design / Plan

### Constraints

- Follow ADR-0003 frame-graph contracts
- Keep node reflection purely declarative (no imperative code in descriptors)
- Enable hot-reload safety per plugin architecture
- Support telemetry integration

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

### Test Plan

- **Documentation Tests:** Verify API contracts cover all node reflection cases
- **Design Review:** Review with rendering-lead for completeness

---

## Steps

1. [x] Create subtask from RG-450
2. [ ] Document node descriptor structures in design doc or header
3. [ ] Define INode interface with reflection, compile, execute contracts
4. [ ] Create example node implementations (deferred G-buffer, lighting, post-process)
5. [ ] Review API with rendering-lead and module leads
6. [ ] Mark RG-450-B (resource registry) as ready
7. [ ] Update parent RG-450 with progress

---

## Evidence

**Documentation:** Link to node API design document once created

**Status:** Ready for work
---
id: RT-410-A
title: Stage planner API design
status: ready
priority: P1
area: runtime
size: S
owner: runtime-lead
gates: [docs]
relates_to: [bundle:B, RT-410]
blocked_on: []
links: ["docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md", "hybrid_workflow/backlog/RT-410-runtime-stage-planner.md"]
---

# Task RT-410-A — Stage Planner API Design

## Intent

Design and document the stage planner API contracts so presentation backends and tooling can integrate consistently.

---

## Context

**Current State:**
- RT-410 parent task defines overall runtime stage planner goals
- API contracts need design before implementation can proceed

**Desired State:**
- Stage planner interface documented with clear contracts
- Presentation backend abstraction defined
- Integration points for tooling identified

**References:**
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [`hybrid_workflow/backlog/RT-410-runtime-stage-planner.md`](RT-410-runtime-stage-planner.md)

---

## Design / Plan

### Constraints

- Follow ADR-0008 runtime loop architecture
- Maintain determinism across headless and presentation modes
- Keep API minimal and testable

### API / Data Sketch

```cpp
namespace engine::runtime {

struct StageHandle {
  std::string_view name;
  RuntimeStageKind kind;
  Duration budget;
};

class RuntimeStagePlanner {
public:
  Result<void, ErrorCode> ConfigurePlan(const RuntimeLoopPlan& plan);
  Result<StageExecution, ErrorCode> NextStage(const RuntimeContext& ctx);
};

} // namespace engine::runtime
```

### Test Plan

- **Documentation Tests:** Verify API contracts are complete and testable
- **Design Review:** Review with runtime-lead and rendering-lead for integration points

---

## Steps

1. [x] Create subtask from RT-410
2. [ ] Document stage planner API in ADR-0008 or new design doc
3. [ ] Define StageHandle, RuntimeStagePlanner, PresentationBackend interfaces
4. [ ] Identify integration points for PM-510 demos
5. [ ] Review API with module leads
6. [ ] Mark RT-410-B (presentation backend abstraction) as ready
7. [ ] Update parent RT-410 with progress

---

## Evidence

**Documentation:** Link to API design document once created

**Status:** Ready for work

