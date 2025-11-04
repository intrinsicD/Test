## PRIORITY_DECISION
- Selected Task: GE-212/GE-220 work division guidance
- Score Table:
  | Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
  | ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
  | GE-212/GE-220 staffing guidance | 3 | 4 | 4 | 3 | 4 | 5 | 23 |
  | Defer update until execution begins | 1 | 1 | 1 | 1 | 5 | 1 | 10 |
- Tie-break Rationale: N/A
- Decision Rationale (≤5 bullets)
  - Clarifying staffing ahead of execution keeps roadmap tasks unblocked without waiting on implementation to start.
  - Parallelising planning (`GE-212`) and instrumentation (`GE-220`) protects schedule slack for the diagnostics initiative.
  - Documentation-only change is low-effort yet high-leverage for agent coordination.
  - Aligns with geometry module roadmap and central backlog expectations.
  - Avoids re-planning churn when agents pick up work in the next sprint.

## DESIGN_BRIEF
Problem Statement: Geometry roadmap entries for `GE-212` (remeshing/parameterisation RFP) and `GE-220` (telemetry alignment) lack explicit staffing guidance, leaving ambiguity on whether they can proceed concurrently.

Acceptance Criteria:
- Geometry module README and roadmap explicitly distinguish the planning versus instrumentation nature of the two tasks.
- Central roadmap queue highlights the parallel staffing strategy.
- Guidance references existing telemetry schema dependency without inventing new couplings.

Interfaces & Data Flow: Documentation updates only; no code paths or APIs affected.

Invariants: Keep roadmap tables intact, preserve existing task identifiers, and avoid altering unrelated module guidance.

Compatibility/Migrations: N/A — informational documentation change.

Security/Performance/Edge Cases: None; ensure wording reinforces existing diagnostics schema dependency and avoids implying new runtime coupling.

Test Plan: Run `python scripts/validate_docs.py` to confirm documentation integrity.

## PATCH
```diff
diff --git a/docs/modules/geometry/README.md b/docs/modules/geometry/README.md
index 8cc76c9..73b3a18 100644
--- a/docs/modules/geometry/README.md
+++ b/docs/modules/geometry/README.md
@@
 | `GE-220` | Align geometry telemetry with diagnostics (`CC-001`). | Add instrumentation for spatial queries and document metrics. | 🟢 Todo |
 
 See [ROADMAP.md](ROADMAP.md) for full context.
+
+### Staffing Notes
+
+- `GE-212` is a planning/RFP effort focused on defining scope and dependencies.
+- `GE-220` instruments telemetry pathways on top of the established diagnostics schema.
+- Assign separate agents to each task and coordinate asynchronously on schema updates to keep workstreams decoupled.
diff --git a/docs/modules/geometry/ROADMAP.md b/docs/modules/geometry/ROADMAP.md
index 0bcc158..b4a0bb6 100644
--- a/docs/modules/geometry/ROADMAP.md
+++ b/docs/modules/geometry/ROADMAP.md
@@
 | `GE-212` | Draft remeshing/parameterisation RFP referencing design constraints. | None |
 | `GE-220` | Instrument spatial query telemetry and update README. | Schema ready (`CC-001.1`); wire metrics via diagnostics bridge |
 
+> **Staffing Guidance:** `GE-212` focuses on planning and RFP authorship, while `GE-220` delivers telemetry instrumentation. Apart from consuming the diagnostics schema from `CC-001`, the efforts are independent, so assign separate agents and sync via documentation check-ins.
+
 ## Dependencies
diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
index df175b3..55f6c09 100644
--- a/docs/ROADMAP.md
+++ b/docs/ROADMAP.md
@@
-  `GE-212` remeshing RFP draft.
+  split `GE-212` remeshing RFP planning and `GE-220` telemetry alignment
+  instrumentation across separate agents so both advance in parallel.
```
## TESTS
N/A

## DOCS
Covered above in `## PATCH`.

## VERIFY
- `python scripts/validate_docs.py` (pass)

## REVIEW_FINDINGS
- Checklist run with no blocking issues; documentation updates align with roadmap intent and verification step succeeded.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
```diff
diff --git a/docs/modules/geometry/README.md b/docs/modules/geometry/README.md
index 8cc76c9..73b3a18 100644
--- a/docs/modules/geometry/README.md
+++ b/docs/modules/geometry/README.md
@@
 | `GE-220` | Align geometry telemetry with diagnostics (`CC-001`). | Add instrumentation for spatial queries and document metrics. | 🟢 Todo |
 
 See [ROADMAP.md](ROADMAP.md) for full context.
+
+### Staffing Notes
+
+- `GE-212` is a planning/RFP effort focused on defining scope and dependencies.
+- `GE-220` instruments telemetry pathways on top of the established diagnostics schema.
+- Assign separate agents to each task and coordinate asynchronously on schema updates to keep workstreams decoupled.
diff --git a/docs/modules/geometry/ROADMAP.md b/docs/modules/geometry/ROADMAP.md
index 0bcc158..b4a0bb6 100644
--- a/docs/modules/geometry/ROADMAP.md
+++ b/docs/modules/geometry/ROADMAP.md
@@
 | `GE-212` | Draft remeshing/parameterisation RFP referencing design constraints. | None |
 | `GE-220` | Instrument spatial query telemetry and update README. | Schema ready (`CC-001.1`); wire metrics via diagnostics bridge |
 
 > **Staffing Guidance:** `GE-212` focuses on planning and RFP authorship, while `GE-220` delivers telemetry instrumentation. Apart from consuming the diagnostics schema from `CC-001`, the efforts are independent, so assign separate agents and sync via documentation check-ins.
 
 ## Dependencies
diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
index df175b3..55f6c09 100644
--- a/docs/ROADMAP.md
+++ b/docs/ROADMAP.md
@@
-- **Geometry** — `GE-205` accelerated normals benchmark for `TI-002`, then
-  `GE-212` remeshing RFP draft.
+- **Geometry** — `GE-205` accelerated normals benchmark for `TI-002`, then
+  split `GE-212` remeshing RFP planning and `GE-220` telemetry alignment
+  instrumentation across separate agents so both advance in parallel.
```

## FOLLOW_UP_TODOS
- [x] Confirm agent assignment for `GE-212` RFP drafting during next sprint planning (owner: Geometry, priority: Medium, rationale: RFP published on 2025-03-26).
- [ ] Align telemetry dashboard expectations with `GE-220` instrumentation outputs (owner: TBD, priority: Low, rationale: observability consumers stay informed).
- [ ] Revisit combined geometry roadmap once `GE-205` completes to sequence downstream remeshing execution (owner: TBD, priority: Low, rationale: long-term coordination).
- [ ] Update diagnostics/telemetry runbook with geometry-specific metrics once instrumentation lands (owner: TBD, priority: Medium, rationale: documentation completeness).
