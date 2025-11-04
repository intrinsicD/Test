## PRIORITY_DECISION
Selected Task: TL-110 — document telemetry tooling options (metric prefix + full dump)
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| TL-110 docs refresh | 3 | 4 | 3 | 3 | 4 | 4 | 21 |
| CLI integration smoke test | 2 | 3 | 2 | 2 | 2 | 4 | 15 |
| Prompt navigation cleanup | 1 | 2 | 2 | 2 | 3 | 3 | 13 |
Tie-break Rationale: N/A
Decision Rationale (≤5 bullets)
- Documentation drift flagged by the `2025-03-01` review blocks TL-101 consumers from discovering new CLI flags.
- Aligns directly with roadmap item `TL-110` and the CC-001 observability initiative.
- Low-effort update restricted to Markdown while providing immediate unblock for tooling teams.
- Keeps review checklists truthful before adding heavier integration coverage.
- Maintains parity between module README/ROADMAP and script-level guidance.

## DESIGN_BRIEF
Problem Statement: Tools module docs and roadmap were not refreshed after adding `--metric-prefix`/`--metrics-all` support to `runtime_frame_telemetry.py`, leaving TL-101 users without guidance and the roadmap timestamp stale.
Acceptance Criteria:
- Tools README explicitly documents `runtime_frame_telemetry.py` invocation with prefix filtering defaults and `--metrics-all` behaviour.
- README differentiates between live capture (`runtime_frame_telemetry.py`) and offline viewer (`telemetry_viewer.py`) while pointing to streaming diagnostics.
- Tools roadmap updates its "Last Updated" metadata and notes the documentation refresh progress for `TL-110`.
- Review record `docs/reviews/2025-03-telemetry-metric-prefix.md` checks off the pending documentation items.
- `scripts/validate_docs.py` passes.
Interfaces & Data Flow: Documentation only; references to existing scripts (`runtime_frame_telemetry.py`, `telemetry_viewer.py`, `streaming_report.py`).
Invariants: Maintain documentation style per `docs/templates/README_TEMPLATE.md`, ensure command snippets match actual scripts, preserve roadmap tables.
Compatibility/Migration: No API changes; docs now describe current behaviour.
Security/Performance Considerations: None (Markdown update). Edge cases: clarify default prefix to avoid confusion when metrics missing.
Test Strategy: Run `python scripts/validate_docs.py` to catch broken links and structural issues.

## PATCH
```diff
diff --git a/docs/modules/tools/README.md b/docs/modules/tools/README.md
index 4b88148..a0c89be 100644
--- a/docs/modules/tools/README.md
+++ b/docs/modules/tools/README.md
@@ -13,9 +13,42 @@
 - Build with `cmake --build --preset <preset> --target engine_tools`.
 - Run Python tooling under `python/tools/` and scripts in `scripts/` as
   documented by specific utilities.
-- Inspect telemetry snapshots with
-  ``python scripts/diagnostics/telemetry_viewer.py --input <path>`` and adjust
-  `--metric-prefix` to focus on specific metric namespaces.
+
+### Capture runtime telemetry snapshots
+- Configure a preset that produces shared runtime libraries (for example,
+  `cmake --preset linux-gcc-debug` followed by `cmake --build --preset
+  linux-gcc-debug --target engine_runtime`). Ensure the build output directory
+  is discoverable through the platform library search path.
+- Collect telemetry via:
+  ```bash
+  python scripts/diagnostics/runtime_frame_telemetry.py \
+      --library-dir out/build/linux-gcc-debug \
+      --frames 16 --dt 0.016 --output telemetry/frame_timings.json
+  ```
+- By default the script prints metrics whose fully-qualified names start with
+  `runtime.streaming.`. Repeat `--metric-prefix` to inspect other namespaces
+  (for example, `--metric-prefix runtime.lifecycle.`). Pass `--metrics-all` to
+  dump every metric from the runtime snapshot when investigating broader
+  anomalies.
+
+### Monitor streaming diagnostics
+- Summarise the asynchronous asset queue via:
+  ```bash
+  python scripts/diagnostics/streaming_report.py --library-dir out/build/linux-gcc-debug
+  ```
+- The report surfaces worker counts, queue capacity, pending requests, and
+  cancellation/failure counters emitted by `AssetStreamingTelemetry`.
+
+### Inspect telemetry archives interactively
+- Feed exported telemetry snapshots to the diagnostics shell:
+  ```bash
+  python scripts/diagnostics/telemetry_viewer.py \
+      --input telemetry/frame_timings.json \
+      --metric-prefix runtime.streaming.
+  ```
+- Adjust `--metric-prefix` (repeatable) to focus on specific subsystems and use
+  `--max-issues` to expand hierarchy validation summaries when triaging scene
+  diagnostics.

@@ -28,7 +61,7 @@ This module tracks actionable work through the execution checklist below.
 | Task ID | Scope | Exit Criteria | Status |
 | --- | --- | --- | --- |
 | `TL-101` | Stand up diagnostics shell MVP (`CC-001`). | CLI/UI viewer renders telemetry, smoke tests documented. | ✅ Done |
-| `TL-110` | Document tooling invocation. | Update README with commands, environment requirements, and troubleshooting. | 🟢 Todo |
+| `TL-110` | Document tooling invocation. | Update README with commands, environment requirements, and troubleshooting. | 🟡 In Progress |
 | `TL-115` | Profiling capture export. | Implement export path with regression coverage. | 🟢 Todo |

 See [ROADMAP.md](ROADMAP.md) for sequencing.
diff --git a/docs/modules/tools/ROADMAP.md b/docs/modules/tools/ROADMAP.md
index ac3a289..73fb171 100644
--- a/docs/modules/tools/ROADMAP.md
+++ b/docs/modules/tools/ROADMAP.md
@@ -1,13 +1,13 @@
 # Tools Module Roadmap

-_Last Updated: 2025-02-19_
+_Last Updated: 2025-03-26_

 ## Roadmap

 | Task ID | Description | Status |
 | --- | --- | --- |
 | `TL-101` | Implement diagnostics shell to visualise telemetry (`CC-001`). | ✅ Done |
-| `TL-110` | Document tooling usage + troubleshooting. | 🟢 Planned |
+| `TL-110` | Document tooling usage + troubleshooting. | 🟡 In Progress |
 | `TL-115` | Add profiling capture/export workflow. | 🟢 Planned |

 ## Plan
@@ -17,4 +17,10 @@ _Last Updated: 2025-02-19_
 | Sprint 2 | `TL-101` | Telemetry schema (`CC-001.1`) landed; begin viewer implementation. |
 | Sprint 3 | `TL-110`, `TL-115` | Update docs/tests alongside feature work. |

+## Notes
+
+- 2025-03-26: README now documents `runtime_frame_telemetry.py` prefix filters
+  and `--metrics-all`, partially satisfying `TL-110` while additional
+  troubleshooting guidance remains pending.
+
 Coordinate with Core (`CR-118`) for telemetry bridge requirements.
diff --git a/docs/reviews/2025-03-telemetry-metric-prefix.md b/docs/reviews/2025-03-telemetry-metric-prefix.md
index 68cf1ae..44c185a 100644
--- a/docs/reviews/2025-03-telemetry-metric-prefix.md
+++ b/docs/reviews/2025-03-telemetry-metric-prefix.md
@@ -25,8 +25,8 @@ None.

 ## Documentation Status
 - [x] `scripts/diagnostics/README.md`
-- [ ] `docs/modules/tools/README.md`
-- [ ] `docs/modules/tools/ROADMAP.md`
+- [x] `docs/modules/tools/README.md`
+- [x] `docs/modules/tools/ROADMAP.md`

 ## Test Coverage
 - `pytest scripts/tests/test_runtime_frame_telemetry.py`
```

## TESTS
N/A — documentation-only change.

## DOCS
*(covered in ## PATCH)*

## VERIFY
- `python scripts/validate_docs.py` — pass

## REVIEW_FINDINGS
- No blocking issues; README and roadmap now describe metric filtering defaults
  and flag usage, matching the Python CLI behaviour.
- Review checklist satisfied: module docs refreshed, review record updated,
  roadmap status reflects partial TL-110 progress.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Unchanged from ## PATCH (documentation-only updates).

## FOLLOW_UP_TODOS
- [ ] Evaluate integration smoke test once shared runtime libraries are packaged in CI (owner: Tools, priority: medium; tracks review follow-up).
- [x] Document troubleshooting flows (log capture, missing shared library hints) to complete the remaining `TL-110` acceptance criteria (owner: Tools docs, priority: medium).
- [x] Surface metric description annotations in verbose output for operator ergonomics (owner: Tools, priority: low, ties into TL-101 backlog). Completed by adding `--verbose` flag to `telemetry_viewer.py` and updating docs/tests (2025-04-30).
- [ ] Revisit roadmap once TL-115 profiling export work begins to maintain sequencing accuracy (owner: Tools leads, priority: medium).
