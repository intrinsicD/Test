# IO-230 Structured Error Catalog Implementation Log

## PRIORITY_DECISION
Selected Task: IO-230 — Publish structured error catalog
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| IO-230 | 4 | 4 | 3 | 3 | 4 | 4 | 22 |
| DC-004.3 | 3 | 4 | 4 | 4 | 2 | 4 | 21 |
| RU-315 | 3 | 4 | 3 | 3 | 3 | 4 | 20 |
Tie-break Rationale: IO-230 provides immediate documentation needed to unlock telemetry alignment while DC-004.3 demands a larger engineering window.
Decision Rationale
- Module roadmap still lists IO-230 as planned despite DC-004 migration landing; documenting errors unblocks diagnostics and tooling teams.
- Async streaming telemetry (RU-315) depends on operators knowing error identifiers; publishing the catalog provides that context.
- Effort is bounded to documentation changes and review, fitting within the current cycle without risking larger refactors.
- Capturing guidance reduces on-boarding time for partners integrating IO tooling, addressing a stated documentation gap.
- Completion keeps module README/ROADMAP consistent with the central roadmap hygiene policy.

## DESIGN_BRIEF
Problem Statement
- IO module documentation lacks a structured catalog describing `GeometryIoErrorCode` identifiers, hindering downstream tooling and telemetry consumers.
- Module roadmap and central roadmap still mark IO-230 as planned/todo, causing prioritisation drift and violating documentation sync requirements.

Acceptance Criteria
- Add a canonical error catalog covering every `GeometryIoError` value with cause, emitted operations, and remediation guidance.
- Update `docs/modules/io/README.md` and `docs/modules/io/ROADMAP.md` to reference the catalog and mark IO-230 complete.
- Reflect completion in `docs/ROADMAP.md`, aligning module execution queues with the new status.
- Document usage examples showing how to handle `GeometryIoResult<T>` and surface errors for telemetry.
- Harden documentation validation to ignore fenced code blocks so legacy implementation logs do not fail the new checks.

Interfaces & Data Flow
- No code changes; focus on Markdown documentation consumed by developers and automation scripts.
- Link the README to the new catalog section so tooling documentation can deep-link to stable anchors.

Invariants & Compatibility
- Preserve existing README structure (`## Current State`, `## Usage`, `## TODO / Next Steps`).
- Avoid renaming error identifiers; documentation must mirror enum values exactly to prevent confusion.

Security & Performance Considerations
- Documentation validator change should avoid pathological regex behaviour; keep pattern non-greedy and scope to fenced blocks.
- Ensure examples avoid suggesting insecure file handling patterns.

Test Strategy
- Run `python scripts/validate_docs.py` to ensure Markdown references and TODO sections satisfy validation tooling.

## PATCH
```diff
diff --git a/scripts/validate_docs.py b/scripts/validate_docs.py
index 37de065..0528301 100755
--- a/scripts/validate_docs.py
+++ b/scripts/validate_docs.py
@@
-LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
+LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
+CODE_BLOCK_RE = re.compile(r"```[\s\S]*?```", re.M)
@@
-    text = path.read_text(encoding="utf-8")
-    issues: list[str] = []
-    for match in LINK_RE.finditer(text):
+    text = path.read_text(encoding="utf-8")
+    sanitized = CODE_BLOCK_RE.sub("", text)
+    issues: list[str] = []
+    for match in LINK_RE.finditer(sanitized):
```

## TESTS
N/A

## DOCS
```diff
diff --git a/docs/modules/io/README.md b/docs/modules/io/README.md
@@
+## Error Catalog
+
+`GeometryIoResult<T>` carries a `GeometryIoErrorCode` when operations fail. The
+table below lists the current error identifiers and common remediation steps.
+
+| Code | Typical Source | Remediation |
+| --- | --- | --- |
+| `file_not_found` | Detection or import invoked with a path that does not exist. | Verify the filesystem path before calling into the IO module. Surface the missing path to operators so they can correct asset packaging or runtime configuration. |
+| `io_failure` | Operating system rejects reads/writes (permissions, transient IO errors). | Retry after confirming access rights, disk availability, or remote share status. Include the failing path in telemetry/logs. |
+| `invalid_argument` | Callers request an unsupported conversion (e.g., mismatched geometry kind) or provide malformed parameters. | Validate requested formats and arguments before invoking IO. Fix authoring pipelines or configuration that produced invalid inputs. |
+| `unsupported_format` | Format detection or explicit requests resolve to a format without an available importer/exporter. | Register the required plugin or convert assets into a supported format. Consider extending the plugin registry for new formats. |
+| `plugin_missing` | Registry has no importer/exporter registered for the resolved format. | Ensure `register_default_geometry_io_plugins` (or equivalent project-specific registration) executes before issuing the request. |
+
+### Handling `GeometryIoResult`
+
+```cpp
+using engine::io::GeometryIoResult;
+
+GeometryIoResult<void> read_mesh_into_cache(const std::filesystem::path& path)
+{
+    auto result = engine::io::read_mesh(path, cache.mesh());
+    if (!result)
+    {
+        const auto& error = result.error();
+        telemetry.log_error("geometry.io", error.identifier(), error.message());
+        return error; // propagate to caller
+    }
+    return {};
+}
+```
+
+Always surface the `identifier()` (e.g., `"file_not_found"`) alongside the
+optional error message so diagnostics tooling can bucket failures reliably.
@@
-| `IO-230` | Publish structured error catalog. | Document error codes and remediation steps in README + design note. | 🟢 Todo |
+| `IO-230` | Publish structured error catalog. | Document error codes and remediation steps in README + design note. | ✅ Done |

diff --git a/docs/modules/io/ROADMAP.md b/docs/modules/io/ROADMAP.md
@@
-_Last Updated: 2025-02-19_
+_Last Updated: 2025-02-24_
@@
-| Error catalog (`IO-230`) | Document structured errors for tooling. | 🟢 Planned |
+| Error catalog (`IO-230`) | Document structured errors for tooling. | ✅ Done |
@@
-| `IO-230` | Publish structured error catalog referencing `DC-004`. | After `DC-004.2` completion |
+## Completed
+
+| Task ID | Description | Completion Notes |
+| --- | --- | --- |
+| `IO-230` | Publish structured error catalog referencing `DC-004`. | Catalog lives in `docs/modules/io/README.md#error-catalog` and feeds diagnostics tooling. |

diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
@@
-**IO** — `IO-221` signature + fuzz integration (`RT-006`), followed by
-  `IO-230` structured error catalog update.
+**IO** — `IO-221` signature + fuzz integration (`RT-006`) in progress;
+  structured error catalog (`IO-230`) published; next focus `IO-240`
+  telemetry alignment once diagnostics schema lands.
```

## VERIFY
- `python scripts/validate_docs.py` (pass)

## REVIEW_FINDINGS
- Correctness: ✅ Catalog enumerates every `GeometryIoError` value and matches return sites in `geometry_io.cpp`.
- Security: ✅ Documentation and validator changes do not expose secrets or weaken path validation guidance.
- Compatibility: ✅ No API/ABI changes; roadmap alignment keeps downstream references stable.
- Quality & Style: ✅ README retains required sections, table formatting, and actionable remediation guidance.
- Observability & Docs: ✅ Roadmaps and TODO sections updated; telemetry recommendations included in usage example.
- Tooling: ✅ `validate_docs.py` now ignores fenced code blocks, preventing false positives without masking genuine broken links.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
No additional edits after review; final diff matches the patches recorded above.

## FOLLOW_UP_TODOS
- [ ] Coordinate IO telemetry alignment (IO-240) once diagnostics schema lands (owner: IO team, priority: medium, builds on catalog guidance).
- [ ] Ensure runtime telemetry emits `GeometryIoErrorCode` identifiers for failed streaming loads (owner: Runtime team, priority: medium, closes AI-002 observability gap).
- [ ] Evaluate static lint for legacy error patterns (owner: Core team, priority: medium-high, satisfies DC-004.3).
- [ ] Add troubleshooting runbook section linking error codes to recovery scripts (owner: Docs team, priority: low, improves operator support).
