# Review: Telemetry metric prefix filtering (2025-03-01)

Following the checklist in [`docs/prompts/review-checklist.md`](../prompts/review-checklist.md).

## Summary
Extends the Python diagnostics viewer to filter runtime telemetry metrics by prefix, exposes an opt-in flag to dump all metrics, and documents the new workflow so TL-101 tooling can inspect lifecycle, stage, and subsystem data.

## Architectural Impact
- Python-only change; no engine binaries touched.
- Supports roadmap item `CC-001` / Tools `TL-101` by improving diagnostics consumption.
- Maintains telemetry schema contract defined in `engine::core` and runtime API.

## Findings

### Critical Issues 🔴
None.

### Warnings ⚠️
None.

### Suggestions 💡
1. Consider surfacing metric descriptions in verbose mode to help operators interpret unfamiliar counters.
   - Rationale: reduces lookup time when new emitters appear.
   - Follow-up task: incorporate into TL-101 viewer backlog if console output remains primary interface.

## Documentation Status
- [x] `scripts/diagnostics/README.md`
- [ ] `docs/modules/tools/README.md`
- [ ] `docs/modules/tools/ROADMAP.md`

## Test Coverage
- `pytest scripts/tests/test_runtime_frame_telemetry.py`

## Follow-Up Work
- [ ] Evaluate integration test coverage that runs the CLI against a real runtime build once shared libraries are available in CI.

## Verdict
- [x] ✅ Approve
