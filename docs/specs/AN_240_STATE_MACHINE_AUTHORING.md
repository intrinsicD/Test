# AN-240: Animation State-Machine Authoring Specification

- **Status:** Draft
- **Owners:** Animation Tools Working Group
- **Last Updated:** 2025-03-24
- **Related Tasks:** `AN-240`, depends on benchmarking assumptions from `AN-230`

## Context

Existing animation controllers in the engine are clip-centric with limited
support for branching logic. Authoring state machines requires bespoke code,
leading to divergent behaviours between runtime and tooling. Roadmap item
`AN-240` introduces a standardized authoring model that unifies how designers
compose states, transitions, and event-driven behaviours while preserving the
runtime guarantees outlined in `ADR-0006`.

## Goals

1. Provide a deterministic authoring format for animation state machines that
   maps 1:1 to runtime execution graphs.
2. Enable tooling to preview transitions and evaluate blending policies
   without embedding engine runtime components.
3. Support data-driven extendability for custom conditions, events, and
   transition effects authored by external teams.
4. Align telemetry and debugging hooks with diagnostics initiatives (`CC-001`).

## Non-Goals

- Implementing the runtime executor (tracked separately under AN-241+).
- Shipping an interactive GUI; this spec covers data model and APIs for
  command-line tooling and DCC integrations.
- Revisiting deformation math or GPU sampling kernels (covered by other
  roadmap items).

## Terminology

- **State Node:** Represents a playable animation clip, blend tree, or nested
  sub-state machine.
- **Transition:** Directed edge evaluated when source state publishes an exit
  event and guard conditions succeed.
- **Condition:** Boolean expression referencing parameters, events, or runtime
  telemetry (e.g., speed, surface contact).
- **Event Channel:** Structured message bus carrying inputs from gameplay or
  physics systems.
- **Action:** Hook executed on transition entry/exit for side effects (e.g.,
  parameter resets, event emission).

## Authoring Format

State machines are serialized into a versioned JSON document stored under
`assets/animation/state_machines/<name>.anm-sm.json`. Schema elements:

```json
{
  "version": 1,
  "name": "locomotion_base",
  "parameters": {
    "speed": { "type": "float", "default": 0.0, "range": [0.0, 10.0] },
    "is_grounded": { "type": "bool", "default": true }
  },
  "states": {
    "idle": { "type": "clip", "clip": "humanoid_idle" },
    "walk": { "type": "blend_tree", "tree": "humanoid_walk_bt" }
  },
  "transitions": [
    {
      "from": "idle",
      "to": "walk",
      "conditions": ["speed > 0.2", "is_grounded"],
      "blend": { "duration": 0.25, "curve": "ease_out" },
      "events": { "on_enter": ["emit:footstep"], "on_exit": [] }
    }
  ]
}
```

### Validation Rules

- Document must declare a single start state and optional entry actions.
- Parameter references must exist and respect declared types/ranges.
- Transition guards compile into bytecode executed by the runtime evaluator.
- Blend policies reference predefined curves stored in
  `assets/animation/blend_curves/`.
- Nested state machines reference other JSON documents with compatible
  versions.

## Runtime Integration Points

- Parsing occurs in `engine/animation/state_machine/loader.hpp`, emitting a
  `StateMachineDefinition` with immutable data structures.
- Execution uses a stack-based interpreter that mirrors the JSON order for
  determinism. Transition evaluation occurs once per frame per active state.
- Parameter updates originate from gameplay or physics callbacks, queued in a
  lock-free ring buffer shared with the animation thread.
- Exit actions publish events onto the runtime event bus, which must honour the
  compute queue assumptions shared with `AN-230` when GPU sampling is active
  (i.e., transitions cannot stall waiting for GPU fences; they observe
  `KernelDispatcher` completion callbacks instead).

## Tooling Workflow

1. **Authoring** — Designers edit JSON via structured editors (VS Code schema,
   scripted exporters, or eventual GUI). Schema is enforced by
   `python/tools/validate_state_machine.py`.
2. **Preview** — `engine/animation/tools/state_machine_preview` loads the JSON,
   plays through transitions using recorded input traces, and visualises
   blending curves.
3. **Publishing** — Build pipeline validates JSON, generates binary cache files
   (`.anm-smc`) with hashed dependencies, and uploads metadata to telemetry.

## Telemetry & Debugging

- Each state machine instance emits counters for active states, transition
  success rates, and guard evaluation timings.
- Failures (missing clips, invalid parameters) raise structured errors tagged
  with machine name and asset GUID.
- Benchmark harness from `AN-230` records transition latency to ensure compute
  queue synchronization keeps pacing jitter below 1 ms.

## Extensibility

- Conditions compile to a DSL backed by a library of primitive operators
  (comparison, boolean logic, math) and can load plugins from
  `engine/animation/state_machine/conditions/`.
- Actions support built-in types (parameter set, event emit, telemetry mark) and
  plugin hooks registered at startup.
- Versioned schema allows additive changes; breaking changes require bumping the
  `version` field and providing migration scripts.

## Deliverables

- JSON schema definition committed under `docs/schemas/animation/state_machine.schema.json`.
- Loader and preview tool stubs in the animation module.
- Authoring guide update in `docs/modules/animation/README.md` referencing this
  specification.
- Task checklist updates linking runtime work (AN-241) once implementation
  begins.

## Open Issues

- Determining DSL expressiveness vs. evaluation speed for complex conditions.
- Persisting editor-specific metadata without polluting runtime cache files.
- Coordinating event channel naming with gameplay module conventions.
