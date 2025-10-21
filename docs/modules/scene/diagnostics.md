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
- Coordinate with the runtime team on `SC-230` to define alert thresholds
  for repeated hierarchy failures. Capture decisions in this guide when
  the alerting policy stabilises.
- Persist telemetry artefacts in CI by storing the JSON output from the
  diagnostics scripts. Use consistent filenames (e.g.,
  `scene_validation_<timestamp>.json`) to simplify trend analysis.

## Sample Library (Planned)

`SC-225` will introduce runnable samples that exercise the validation
workflow end-to-end. Once merged, link the fixtures here with setup
instructions and expected telemetry snapshots.

## Related References

- [`docs/modules/runtime/diagnostics.md`](../runtime/diagnostics.md)
- [`docs/prints/sc-220-documentation-refresh.md`](../../prints/sc-220-documentation-refresh.md)
- [`docs/prints/rt-005-3-hierarchy-diagnostics-docs.md`](../../prints/rt-005-3-hierarchy-diagnostics-docs.md)
- [`docs/design/telemetry_schema.md`](../../design/telemetry_schema.md)
- [`docs/tasks/T-0104-runtime-frame-graph-integration.md`](../../tasks/T-0104-runtime-frame-graph-integration.md)
