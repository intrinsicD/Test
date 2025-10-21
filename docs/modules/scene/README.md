# Scene Module

## Current State
- Provides entity façade with hierarchy management, transform propagation,
  deterministic serialization/deserialization, and component helpers consumed by
  runtime systems.
- `SceneGraphValidator` exposes a fast cycle-detection check returning
  structured `SceneGraphErrorCode` values for diagnostics pipelines.

## Usage
- Build via `cmake --build --preset <preset> --target engine_scene`.
- Include `<engine/scene/scene_graph.hpp>` for hierarchy utilities.
- Include `<engine/scene/graph/scene_graph_validator.hpp>` for cycle detection checks.
- Run `ctest --preset <preset> --tests-regex engine_scene`.

## Diagnostics & Validation Workflow
- Use `scene::validation::validate_hierarchy` (mirrored through
  `RuntimeHost::diagnostics().scene_validation`) to audit imported or
  procedurally-generated hierarchies before submitting to rendering or
  saving authored scenes. The runtime diagnostics guide documents the
  full [Hierarchy Diagnostics Playbook](../runtime/diagnostics.md#hierarchy-diagnostics-playbook)
  with CLI tooling and remediation strategies shared across modules.
- Capture structured telemetry by running
  `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>`
  against a runtime-enabled build. The JSON output includes the latest
  hierarchy validation report so scene tooling can correlate issue types
  with the authoring context that produced them.
- Register listeners with
  `runtime::RuntimeDiagnosticsBridge::register_listener` (see the runtime
  diagnostics guide) when editor integrations require live feedback.
  Callbacks execute on the runtime thread—log or enqueue work instead of
  blocking to preserve determinism.
- Record remediation steps for common issue codes (`DuplicateRoot`,
  `MissingParent`, `CycleDetected`, `InvalidTransform`) in scene tooling so
  authors can resolve problems without cross-referencing runtime internals.
- Consult [diagnostics.md](diagnostics.md) for the complete scene-focused
  remediation guide and telemetry alignment notes introduced by `SC-220`.

## TODO / Next Steps

- Prepare runnable scene import samples that exercise the hierarchy
  diagnostics workflow end-to-end and link them from this README once the
  fixtures land (`SC-225`, planned); track progress in the `RT-005`
  tranche via the
  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
- Coordinate with runtime/tooling owners to define alert thresholds for
  recurring hierarchy failures so dashboards surface sustained
  regressions (`SC-230`, planned); status captured under `RT-005` in the
  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `SC-208` | Implement cycle detection validation (`RT-005`). | Scene validator detects cycles with structured errors and tests. | ✅ Done |
| `SC-215` | Runtime diagnostics bridge. | Runtime reports invalid hierarchies via telemetry/logs. | ✅ Done |
| `SC-220` | Documentation refresh. | Update README + troubleshooting guide with validation workflows. | ✅ Done |

Review [ROADMAP.md](ROADMAP.md) for scheduling notes.
