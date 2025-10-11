# First Rendering Milestone Plan

## Objective

Deliver the first on-screen frame produced by the engine by aligning the rendering and runtime roadmaps around a tightly scoped
integration slice. The milestone emphasises the minimum vertical path capable of submitting a renderable scene graph from the
runtime into the rendering frame graph and replaying it through a backend-neutral scheduler.

## Selected TODOs

### 1. Enrich Frame-Graph Resource Descriptors
- **Source backlog:** `engine/rendering/README.md` short-term roadmap.
- **Description:** Extend `FrameGraphResourceInfo` and the associated creation APIs with explicit format, dimension, and usage
  metadata. The enriched descriptors unblock deterministic allocation by backend resource providers and ensure passes publish the
  constraints required for validation.
- **Deliverables:**
  - Schema update across `engine/rendering/resources` and frame-graph construction helpers.
  - Migration guide documenting the new fields and their defaults.
  - Unit coverage that asserts the presence of the metadata in compiled graphs.
- **Dependencies:** Coordinate schema alignment with the assets module so material and shader descriptors carry matching
  requirements.

### 2. Prototype the Reference GPU Scheduler
- **Source backlog:** `docs/rendering/ROADMAP.md` short-term roadmap.
- **Description:** Implement a backend-neutral scheduler that converts compiled frame-graph passes into a linear submission stream
  targeting an abstract command encoder. The prototype validates dependency resolution, transient lifetime management, and queue
  metadata propagation before platform backends exist.
- **Deliverables:**
  - Reference scheduler implementation with stub encoder hooks and logging.
  - Integration tests that run a multi-pass frame graph through the prototype scheduler.
  - Diagnostics surfaced through the existing logging infrastructure to visualise submission order.
- **Dependencies:** Builds directly on the enriched resource descriptors from TODO 1; exposes data needed by runtime scheduling
  hooks.

### 3. Introduce Runtime Render Submission Hooks
- **Source backlog:** `docs/design/runtime_plan.md` mid-term roadmap.
- **Description:** Extend `RuntimeHost` with a render submission interface (e.g., `RenderGraphBuilder`) that packages the current
  scene snapshot, camera parameters, and resource requirements into a form consumable by the rendering scheduler. The runtime
  should own the orchestration of visibility queries and pass registration for the initial vertical slice.
- **Deliverables:**
  - Runtime façade or adapter that marshals scene data into renderable structures.
  - Smoke test exercising animation → physics → runtime submission → rendering scheduler.
  - Documentation updates illustrating the lifecycle from runtime tick to render submission.
- **Dependencies:** Requires TODOs 1 and 2 to define the data contracts accepted by the rendering scheduler.

## Acceptance Criteria

- A single example application can tick the runtime, build a frame graph, and submit it through the reference GPU scheduler
  without backend-specific code.
- CI includes regression coverage for resource descriptor enrichment, scheduler submission ordering, and runtime-to-rendering
  handoff.
- Documentation across runtime and rendering READMEs references this milestone to orient future contributors.
