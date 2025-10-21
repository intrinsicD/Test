## PRIORITY_DECISION
Selected Task: RT-006.3 — IO signature detection documentation
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| RT-006.3 | 4 | 4 | 3 | 3 | 4 | 5 | 23 |
| IO-240 | 2 | 3 | 3 | 2 | 2 | 4 | 16 |
Tie-break Rationale: Documentation task has nearer-term roadmap dependency.
Decision Rationale:
- Fulfils `RT-006.3` exit criteria unblocking `IO-221` fuzzing integration.
- Clarifies detection workflow for geometry + animation consumers ahead of code changes.
- Establishes fuzzing operations guide required before CI enablement.
- Low-effort documentation-first change that can land before harness infra is ready.
- Keeps roadmap/status artefacts in sync with published guidance.

## DESIGN_BRIEF
Problem Statement: The IO module lacks consolidated instructions for detection heuristics, signature catalogue upkeep, and fuzz harness operation, leaving teams without a canonical reference while fuzzing support and signature databases are built.
Acceptance Criteria:
- Publish a dedicated playbook covering detection pipelines, signature catalogue processes, fuzz harness usage, and triage workflows.
- Link the playbook from IO documentation and navigation tables so contributors can find it quickly.
- Update roadmap status to mark `RT-006.3` complete while noting the playbook in module roadmaps.
- Stage a checked-in corpus directory with guidance for future seed management.
Interfaces & Data Flow: Documentation-only change referencing existing APIs (`detect_geometry_file`, `animation::detect_clip_format`) and the fuzz harness (`engine_io_geometry_fuzz`). No runtime interface changes.
Invariants: Preserve existing error handling contracts (`GeometryIoResult`, `AnimationIoResult`) and keep roadmap status tables aligned across README, `docs/ROADMAP.md`, and module docs.
Compatibility/Migrations: N/A for runtime; documentation must reflect the forthcoming signature catalogue without promising unavailable features.
Security/Performance: Emphasise avoiding data leaks in telemetry and managing fuzz resource usage; no runtime impact.
Test Strategy: Run `python scripts/validate_docs.py` to confirm link integrity.

## PATCH
```diff
diff --git a/docs/README.md b/docs/README.md
index 21e0dc5..b1f47b1 100644
--- a/docs/README.md
+++ b/docs/README.md
@@
-| Telemetry instrumentation workflow | [design/telemetry_instrumentation_guide.md](design/telemetry_instrumentation_guide.md) |
+| Telemetry instrumentation workflow | [design/telemetry_instrumentation_guide.md](design/telemetry_instrumentation_guide.md) |
+| IO detection & fuzzing playbook | [modules/io/detection_fuzzing_playbook.md](modules/io/detection_fuzzing_playbook.md) |
```
```diff
diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
index e73126f..9fdc640 100644
--- a/docs/ROADMAP.md
+++ b/docs/ROADMAP.md
@@
-| `RT-006.3` | Author detection docs. | IO README explains detection workflow, failure handling, and fuzzing steps. | 🟢 Todo |
+| `RT-006.3` | Author detection docs. | IO README explains detection workflow, failure handling, and fuzzing steps. | ✅ Done |
@@
-- **IO** — `IO-221` signature + fuzz integration (`RT-006`) in progress;
+- **IO** — `IO-221` signature + fuzz integration (`RT-006`) in progress; detection
+  & fuzzing playbook published (`RT-006.3`).
```
```diff
diff --git a/docs/modules/io/README.md b/docs/modules/io/README.md
index eed7f3f..3addcbc 100644
--- a/docs/modules/io/README.md
+++ b/docs/modules/io/README.md
@@
-- Run `ctest --preset <preset> --tests-regex engine_io` and fuzz harnesses as
-  they come online.
+- Run `ctest --preset <preset> --tests-regex engine_io` and fuzz harnesses as
+  they come online. Follow the
+  [IO Signature Detection & Fuzzing Playbook](detection_fuzzing_playbook.md)
+  for setup, corpus management, and triage procedures.
@@
-- Track `IO-221` and `IO-240` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — advances `RT-006` and `DC-004`. `IO-230` is now covered by the error catalog above.
+- Track `IO-221` and `IO-240` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — advances `RT-006` and `DC-004`. `IO-230` is now covered by the error catalog above.
+- Refer to the [detection & fuzzing playbook](detection_fuzzing_playbook.md) when onboarding
+  new formats or refreshing the fuzzing harness.
```
```diff
diff --git a/docs/modules/io/ROADMAP.md b/docs/modules/io/ROADMAP.md
index 2ecda2a..d3c6ecb 100644
--- a/docs/modules/io/ROADMAP.md
+++ b/docs/modules/io/ROADMAP.md
@@
-# IO Module Roadmap
-
-_Last Updated: 2025-02-24_
+# IO Module Roadmap
+
+_Last Updated: 2025-03-24_
@@
-- Coordinate fuzz harness resource usage with CI owners before enabling.
+- Coordinate fuzz harness resource usage with CI owners before enabling.
+- Detection & fuzzing playbook published (`RT-006.3`); keep it synced when
+  catalogue or harness steps change.
```
```diff
diff --git a/docs/modules/io/detection_fuzzing_playbook.md b/docs/modules/io/detection_fuzzing_playbook.md
new file mode 100644
--- /dev/null
+++ b/docs/modules/io/detection_fuzzing_playbook.md
@@
+# IO Signature Detection & Fuzzing Playbook
+
+## Purpose
+Roadmap task `RT-006.3` delivers a repeatable playbook for IO signature
+detection and fuzzing. The goal is to keep geometry and animation importers
+predictable while new formats and datasets arrive. This guide explains how the
+current detectors work, how to extend the signature catalogue, and how to
+operate the fuzzing harnesses that guard the code paths.
+
+## Scope & Prerequisites
+- Covers the geometry and animation detection entry points:
+  - `engine::io::detect_geometry_file` in
+    [`engine/io/src/geometry_io.cpp`](../../../engine/io/src/geometry_io.cpp)
+    controls mesh/point cloud/graph sniffing.
+  - `engine::io::animation::detect_clip_format` in
+    [`engine/io/src/animation_importer.cpp`](../../../engine/io/src/animation_importer.cpp)
+    resolves clip formats before loading.
+- Applies to the libFuzzer harness defined in
+  [`engine/io/tests/geometry_io_detection_fuzz.cpp`](../../../engine/io/tests/geometry_io_detection_fuzz.cpp).
+- Requires a C++20 toolchain with sanitizer + libFuzzer support when fuzzing.
+  Pass `-DENGINE_ENABLE_FUZZING=ON` to `cmake` to build fuzz targets.
+- Always run `python scripts/validate_docs.py` after editing this guide or
+  linked READMEs to keep navigation intact.
+
+## Detection Workflow
+...
```
```diff
diff --git a/engine/io/tests/corpus/geometry_detection/README.md b/engine/io/tests/corpus/geometry_detection/README.md
new file mode 100644
--- /dev/null
+++ b/engine/io/tests/corpus/geometry_detection/README.md
@@
+# Geometry Detection Fuzz Corpus
+
+Store minimal, provenance-tracked seeds that exercise the geometry detection
+harness (`engine_io_geometry_fuzz`). Keep entries small (<64 KiB) and document
+format, intent, and source in this file. Update the table below whenever seeds
+change.
+
+| File | Format | Intent | Provenance |
+| --- | --- | --- | --- |
+| _TBD_ | – | Placeholder entry until curated seeds land with `RT-006.1`. | – |
+
```

## TESTS
N/A — documentation-only change.

## DOCS
Included in ## PATCH (all touched files are documentation assets).

## VERIFY
- `python scripts/validate_docs.py` — all references resolved.

## REVIEW_FINDINGS
- Correctness: Playbook accurately reflects current detection flow (`detect_geometry_file`, `detect_clip_format`).
- Security: Guidance emphasises hashing data before logging; no leaks identified.
- Compatibility: Roadmap/README statuses updated consistently; no API promises altered.
- Quality & Style: Markdown adheres to repository conventions; navigation entry added.
- Performance: Fuzzing instructions include resource limits; no regressions expected.
- Tests: Link validation executed; no automated tests required for docs.
- Observability: Guidance instructs logging identifiers and future telemetry alignment.
- Docs: Cross-references and module roadmap updates present.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Matches ## PATCH (no changes after review).

## FOLLOW_UP_TODOS
- [ ] Land signature catalogue format + loader (`RT-006.1`).
- [ ] Automate periodic fuzz runs in CI once infrastructure is provisioned.
- [ ] Evaluate extending detection heuristics to texture/material assets (long-term IO roadmap).
- [ ] Backfill task docs (`IO-221`) with references to the published playbook.
