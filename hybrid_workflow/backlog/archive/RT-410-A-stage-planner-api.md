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
links: ["docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md", "docs/design/RT_410_STAGE_PLANNER_API.md", "hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md"]
---
# Task RT-410-A — Stage Planner API Design
## Intent
Design and document the stage planner API contracts so presentation backends and tooling can integrate consistently.
## Steps
1. [x] Create subtask from RT-410
2. [x] Document stage planner API in ADR-0008 or new design doc
   - Created `docs/design/RT_410_STAGE_PLANNER_API.md` with comprehensive API specification
3. [x] Define StageHandle, RuntimeStagePlanner, PresentationBackend interfaces
   - Documented `PresentationConfig`, `PresentationFrame`, enhanced `PresentationBackend`
   - Specified `PresentationStageBuilder` and `PresentationSyncSlot` APIs
4. [x] Review API with module leads
   - (2026-03-31) Facilitated async review with rendering, runtime, and tooling leads; resolved
     terminology clarifications inline with the design document and confirmed readiness for
     implementation.
5. [x] Update parent RT-410 with progress
   - (2026-02-19) Propagated documentation updates for scripting synchronization hooks to the
     RT-410 parent task, marking Step 5 complete.

## Progress Notes

**2025-11-06:** API design document completed (`docs/design/RT_410_STAGE_PLANNER_API.md`)
- Defined presentation configuration structures and backend interface extensions
- Specified integration patterns for OpenGL, Vulkan, and Mock backends
- Documented telemetry integration and tooling patterns
- Outlined edge cases (capability mismatch, sync deadlock, headless mode)
- Created implementation checklist with 7 phases
- Ready for review with module leads

**2026-03-31:** Module lead review concluded
- Validated stage sequencing semantics and telemetry capture points with rendering/runtime leads
- Confirmed tooling synchronization handles align with TL-310 editor integration requirements
- Captured terminology adjustments (`PresentationSyncHandle` → `PresentationSyncSlot`) directly in the design doc
- No additional design follow-ups required before implementation

## Result

- Stage planner API is documented in `docs/design/RT_410_STAGE_PLANNER_API.md` with canonical
  `RuntimeStagePlanner`, `PresentationSyncSlot`, and `PresentationBackend` signatures and error
  semantics.
- Presentation lifecycle now covers initialization, overlay composition, submission, and teardown
  across real and headless backends, providing a clear contract for subsequent implementation tasks.
- Telemetry capture points and tooling synchronization hooks align with PM-510 demo requirements and
  TL-310 editor integration goals.

