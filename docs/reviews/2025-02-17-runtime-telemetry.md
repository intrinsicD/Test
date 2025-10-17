# Review: Runtime telemetry tooling (2025-02-17)

Following the checklist in [`docs/prompts/review-checklist.md`](../prompts/review-checklist.md).

## Summary
Adds a Python diagnostics script that records dispatcher timings from the
runtime library, documents usage under `scripts/diagnostics/`, and closes the
remaining sprint 06 telemetry acceptance criterion.

## Architectural Impact
- Touches only scripting and documentation; no changes to runtime or rendering
  module boundaries.
- Supports roadmap items `AI-003` and `RT-003` by providing instrumentation but
  does not modify engine code.

## Findings

### Critical Issues 🔴
None.

### Warnings ⚠️
None.

### Suggestions 💡
1. Capture baseline telemetry artefacts and check them into CI once shared
   build presets stabilize.
   - Rationale: enables automated regression detection based on the JSON output.
   - Follow-up task: extend sprint backlog or create CI ticket once data is
     available.

## Documentation Status
- [x] `docs/tasks/2025-02-17-sprint-06.md`
- [x] `scripts/README.md`
- [x] `scripts/diagnostics/README.md`

## Test Coverage
- `python -m compileall scripts/diagnostics`
- `python scripts/diagnostics/runtime_frame_telemetry.py --help`

## Follow-Up Work
- [ ] Integrate telemetry capture into CI orchestration when runtime shared
      libraries become available in automated builds.

## Verdict
- [x] ✅ Approve
