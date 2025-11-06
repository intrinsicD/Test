---
id: RT-410-A
title: Stage planner API design
status: done
priority: P1
area: runtime
size: S
owner: runtime-lead
gates: [docs]
relates_to: [bundle:B, RT-410]
blocked_on: []
links: ["docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md", "hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md"]
---
# Task RT-410-A — Stage Planner API Design
## Intent
Design and document the stage planner API contracts so presentation backends and tooling can integrate consistently.
## Steps
1. [x] Create subtask from RT-410
2. [x] Document stage planner API in ADR-0008 or new design doc
   - Added dedicated planner and presenter interface section to ADR-0008 detailing lifecycle and
     error handling contracts (`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`).
3. [x] Define StageHandle, RuntimeStagePlanner, PresentationBackend interfaces
   - Captured struct/class definitions with required members and result types so runtime, tooling,
     and scripting consumers share a single contract surface.
4. [x] Review API with module leads
   - Circulated ADR excerpt to runtime/rendering/tooling leads for asynchronous sign-off; tracked
     minor terminology nits inline (no blocking feedback).
5. [x] Update parent RT-410 with progress
   - Logged documentation milestone under RT-410 Step 4 to keep planner rollout timeline in sync.

## Result

- Stage planner API is documented in ADR-0008 with canonical `StageHandle`, `RuntimeStagePlanner`,
  and `PresentationBackend` signatures and error semantics.
- Presentation lifecycle covers initialization, overlay composition, submission, and teardown
  across real and headless backends, providing a clear contract for subsequent implementation
  tasks.
