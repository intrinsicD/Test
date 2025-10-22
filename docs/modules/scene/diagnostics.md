# Scene Diagnostics Guide

## Purpose
`SC-220` closes the documentation gap between the scene module and the
runtime hierarchy diagnostics playbook published during `RT-005.3`. This
guide summarises how scene authors and tooling integrations consume the
validation workflow, where telemetry lives, and how to respond when
issues surface.

Use this document alongside the runtime diagnostics guide
([`docs/modules/runtime/diagnostics.md`](../runtime/diagnostics.md)) to
keep troubleshooting material synchronised across modules.

## Entry Points

### Standalone Validation
- Call `scene::validation::validate_hierarchy(SceneGraphView)` when
  operating purely within scene tooling or import pipelines. The helper
  returns a `HierarchyValidationReport` that mirrors the payload exposed
  through `RuntimeHost::diagnostics().scene_validation`.
- Log or persist `report.issues` (vector of
  `HierarchyValidationIssue`) for downstream tooling. Each issue encodes
  an enum `code`, a human-readable `message`, and optional `context`
  describing the offending entity path.

### Runtime-Driven Validation
- Runtime integrations receive the same report via
  `RuntimeHost::diagnostics().scene_validation`. Access the payload after
  `RuntimeHost::tick()` to ensure the latest hierarchy snapshot is
  available before rendering or serialising state.
- Register callbacks with
  `runtime::RuntimeDiagnosticsBridge::register_listener` when scene
  tooling needs push-based notifications. Callbacks run on the runtime
  thread immediately after validation; forward summaries to asynchronous
  channels to avoid blocking the frame loop.

### Tooling Scripts
- Execute `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>`
  against a runtime-enabled build to capture JSON output containing the
  hierarchy report, stage timings, and streaming metrics. Use
  `--output <path>` to persist artefacts in CI.
- When operating outside the runtime binary, invoke custom tooling that
  wraps `scene::validation::validate_hierarchy` and emits JSON in the same
  schema to keep dashboards consistent.

## Remediation Workflow

1. **Detect failure** — check `HierarchyValidationReport::ok()` after
   validation. Early exit if no issues are present.
2. **Categorise** — inspect each issue's `code` and map it to the table
   below. Consider logging the parent/child identifiers stored in
   `context` for reproducibility.
3. **Respond** — apply the recommended mitigation, re-run validation,
   and persist the before/after telemetry when a regression is detected.
4. **Escalate** — if the same issue reappears across multiple frames or
   tool sessions, capture consecutive reports and share them with the
   owning team. Include the runtime telemetry JSON where available.

### Issue Reference

| Code | Symptom | Mitigation |
| --- | --- | --- |
| `DuplicateRoot` | More than one entity lacks a parent/root entry. | Adjust import pipelines to enforce a single root; attach detached nodes explicitly. |
| `MissingParent` | A child references a missing parent handle. | Rebuild parent indices and refresh handles before mutating transforms. |
| `CycleDetected` | Hierarchy traversal detects a cycle. | Use `scene::HierarchyEditor::set_parent` to re-parent safely and audit editor undo stacks. |
| `InvalidTransform` | Propagated transforms contain NaNs or degenerate scales. | Clamp authoring data, reset invalid nodes, and inspect physics/animation contributors. |

Augment this table when new `HierarchyValidationIssueCode` values land.
Reference `docs/modules/runtime/diagnostics.md` for additional
instrumentation context.

## Telemetry & Alerting

- Runtime telemetry exposes metrics under `runtime.scene_validation.*`
  with counts and last-seen issue codes; scene tooling should subscribe
  to the same schema to maintain parity.
- `runtime.scene_validation.alert_threshold.warning_frames` and
  `runtime.scene_validation.alert_threshold.critical_frames` document the
  runtime's baked-in alert policy (warning after 3 consecutive failing frames,
  critical after 10). Dashboards should page when
  `runtime.scene_validation.alert_level` reaches `2` and file follow-up issues
  when it remains at `1` for longer than a few minutes.
- Monitor `runtime.scene_validation.consecutive_failure_frames` to understand
  current streak length, `runtime.scene_validation.max_consecutive_failure_frames`
  for historical context, and
  `runtime.scene_validation.last_failure_{simulation_time,wall_seconds}` to
  correlate failures with authored actions or CI jobs.
- Persist telemetry artefacts in CI by storing the JSON output from the
  diagnostics scripts. Use consistent filenames (e.g.,
  `scene_validation_<timestamp>.json`) to simplify trend analysis.

## Sample Library

`engine/scene/samples` ships the `scene_hierarchy_diagnostics_sample`
CLI introduced by `SC-225`. It exercises the hierarchy validation
workflow end-to-end with reproducible fixtures and JSON output that
matches the runtime diagnostics schema.

### Building the CLI

```bash
cmake --build --preset <preset> --target scene_hierarchy_diagnostics_sample
```

The executable is written to
`out/build/<preset>/engine/scene/scene_hierarchy_diagnostics_sample`.

### Validating Built-in Fixtures

Two fixtures demonstrate clean and failing hierarchies:

- `valid_hierarchy` — three nodes with consistent transforms and
  relationships.
- `invalid_hierarchy` — introduces cycles, dangling parents, missing
  hierarchy components, and mismatched transforms.

Invoke the CLI directly against the fixtures:

```bash
./out/build/<preset>/engine/scene/scene_hierarchy_diagnostics_sample \
    --sample invalid_hierarchy --pretty --fail-on-issues
```

`--scene <path>` validates external `.scene` files while `--emit-samples`
regenerates the committed fixtures under
`engine/scene/samples/data/`. Use `--list-samples` to enumerate the
available definitions.

### Expected Output

Every run prints a human-readable summary followed by JSON mirroring
`HierarchyValidationReport`. Example output for the
`invalid_hierarchy` fixture:

```json
{
  "ok": false,
  "metrics": {
    "issue_count": 3,
    "cycle_count": 1,
    "dangling_parent_count": 1,
    "missing_parent_hierarchy_count": 1,
    "non_finite_transform_count": 0,
    "transform_mismatch_count": 0
  },
  "issues": [
    {
      "entity": 4,
      "related": 3,
      "type": "missing_parent_hierarchy",
      "message": "Entity 4 references parent 3 that is missing a Hierarchy component"
    },
    {
      "entity": 2,
      "related": 123456789,
      "type": "dangling_parent",
      "message": "Entity 2 references invalid parent 123456789"
    },
    {
      "entity": 0,
      "related": 1,
      "type": "cycle",
      "message": "Cycle detected when traversing parent chain for entity 0 via parent 1"
    }
  ]
}
```

Use these artefacts in documentation, dashboards, and CI pipelines to
compare runtime telemetry against expected diagnostics output.

## Related References

- [`docs/modules/runtime/diagnostics.md`](../runtime/diagnostics.md)
- [`docs/prints/sc-220-documentation-refresh.md`](../../prints/sc-220-documentation-refresh.md)
- [`docs/prints/rt-005-3-hierarchy-diagnostics-docs.md`](../../prints/rt-005-3-hierarchy-diagnostics-docs.md)
- [`docs/design/telemetry_schema.md`](../../design/telemetry_schema.md)
- [`docs/tasks/T-0104-runtime-frame-graph-integration.md`](../../tasks/T-0104-runtime-frame-graph-integration.md)
