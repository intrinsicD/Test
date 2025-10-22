# Geometry IO Error-Handling Migration (2025-03-17)

## Implementation Plan

### Priority Decision

| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| `DC-004.2` – IO APIs on `Result<T>` | 4 | 5 | 4 | 4 | 2 | 5 | 24 |
| `AI-001.2` – Debug handle validation | 2 | 4 | 3 | 3 | 2 | 4 | 18 |
| `RT-003.1` – Runtime submission API alignment | 3 | 4 | 3 | 3 | 2 | 4 | 19 |

Tie-breaker: `DC-004.2` unlocks structured error reporting across assets/runtime and was already marked “In Progress”, so finishing it first provides the largest unblock multiplier.

### Design Summary

- **Problem Statement:** Geometry IO still relied on exceptions for error signalling despite exposing `GeometryIoResult`, creating inconsistent call-site behaviour and preventing telemetry from classifying failures.
- **Acceptance Criteria:** All geometry/point-cloud/graph import/export entry points return `GeometryIoResult`, map file/format issues to typed `GeometryIoErrorCode` values, surface errors through detection helpers, and record coverage for missing files, invalid data, and write failures.
- **Key Changes:**
  - Introduced `GeometryIoException` + `translate_io_exceptions` wrapper to convert internal throws into typed results.
  - Updated plugin interfaces in `geometry_io_registry.hpp` to return `GeometryIoResult<void>` and adjusted default importer/exporter implementations.
  - Propagated `inspect_ply_header` failures via `GeometryIoResult` to callers.
  - Extended tests to cover missing file, invalid mesh, and write-parent failure scenarios.
  - Synchronized documentation/roadmap status for `DC-004` completion.

### Execution Steps

1. Wrap importer/exporter helpers with `GeometryIoException` and convert all `throw std::runtime_error` sites to typed errors.
2. Change registry interfaces to return `GeometryIoResult<void>`; adjust default plugins and top-level IO functions to propagate results.
3. Add helper wrappers catching remaining exceptions and map them to `GeometryIoErrorCode` values.
4. Update regression tests for new error cases and adjust dummy plugins.
5. Refresh roadmap/README/module documentation to mark `DC-004.2` complete.
6. Validate via `engine_io_tests` and `validate_docs.py`.

## Review (per docs/prompts/review-checklist.md)

### Summary
Implemented structured error handling for all geometry IO code paths by routing importer/exporter operations through `GeometryIoResult`, extending tests for failure scenarios, and updating roadmap/docs to mark `DC-004` complete.

### Architectural Impact
- **Invariants:** Aligns with `DC-004` (error handling) and supports telemetry plumbing for IO failures.
- **Modules:** `engine/io`, geometry IO registry/tests, docs (`README`, `docs/ROADMAP.md`, `docs/modules/io/README.md`).
- **Dependencies:** No new third-party code; leverages existing diagnostics primitives.

### Findings

#### Critical Issues 🔴
- None.

#### Warnings ⚠️
- Existing animation/core warnings from unrelated modules observed during build (pre-existing); no new warnings introduced by this patch.

#### Suggestions 💡
- Consider emitting structured telemetry from `translate_io_exceptions` once diagnostics schema expects IO failure metrics.

### Documentation Status
- [x] `docs/modules/io/README.md`
- [x] `docs/ROADMAP.md`
- [x] Root `README.md`
- [ ] Additional specs not required

### Test Coverage
- Unit tests: `engine_io_tests` extended with three new cases covering missing files, invalid meshes, and write failures.
- Integration: N/A (scope limited to IO module).
- Regression: Ensures importer/exporter errors propagate as `GeometryIoErrorCode` values.

### Follow-Up Work
- [ ] Surface IO failure telemetry via diagnostics viewer once `CC-001` work begins (owner TBD).

### Verdict
- ✅ Approve – ready to merge.
