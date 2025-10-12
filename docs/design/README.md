# Design Records

## Current State

- Houses architecture explorations, design rationales, and decision records for the engine.
- Tracks historical context for feature planning and subsystem experiments.

## Usage

- Capture design proposals here before landing major subsystem work.
- Reference these documents in pull requests so reviewers can trace the intent.

## Milestone Rationales

### Rendering/Runtime vertical slice

- The near-term focus documented in [docs/global_roadmap.md](../global_roadmap.md) is to
  deliver a deterministic path from runtime orchestration into the rendering frame
  graph. The accompanying [First Rendering Milestone Plan](first_rendering_milestone.md)
  distils that objective into three bounded TODOs that move in lockstep: enriching
  frame-graph resource descriptors, prototyping the backend-neutral scheduler, and
  introducing runtime submission hooks.
- Capturing the rationale in a dedicated milestone note keeps the rendering and
  runtime teams aligned on scope while other subsystems (animation, physics, geometry)
  continue to act as data providers. The document also records dependencies so each
  deliverable advertises the contracts it introduces to neighbouring modules.
- As those deliverables land, update the milestone record with acceptance outcomes and
  link test coverage so future contributors understand why the slice was scoped this
  way and what regressions it guards against.

## TODO / Next Steps

- Capture outcomes for the rendering/runtime vertical slice once the scheduler prototype
  and submission hooks are implemented, then mirror any process adjustments back into
  [docs/global_roadmap.md](../global_roadmap.md).
- Continue enumerating design rationales for upcoming milestones (physics contact
  manifolds, geometry IO hardening, tooling telemetry) so that every cross-module
  effort references an explicit plan before execution begins.
