# Context Package — PM-520 Backlog and Roadmap Hygiene Remediation

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-03-05-pm-520-backlog-hygiene.md`](../task_briefs/2026-03-05-pm-520-backlog-hygiene.md)
- **Backlog Entry:** [`docs/backlog/active/PM-520-backlog-hygiene-remediation.md`](../../docs/backlog/active/PM-520-backlog-hygiene-remediation.md) — remediation plan outlining archival steps.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L1-L35】
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md) Phase 4 priority table noting PM-520 and remaining active GPU/runtime/tooling items.【F:docs/ROADMAP.md†L64-L103】
- **Workflow Phase:** Phase 2 – Context Assembly (handoff to Specialist Engineer for execution).【F:AGENTS.md†L61-L117】

## 2. Problem Summary
- Current behaviour: Prior to remediation, `docs/backlog/active/` still contained multiple entries marked "Complete" (DC-040, DC-041, RT-320, TL-210, RT-321, AS-330, CC-310, CC-311, PL-240) while roadmap tables continued linking to `backlog/active/` paths, creating drift between authoritative sources. Their archived records preserve the completion status evidence referenced during planning.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L9-L30】【F:docs/backlog/archive/DC-040-ai-004-configuration-schema.md†L1-L42】【F:docs/backlog/archive/RT-320-runtime-prototyping-harness.md†L1-L62】【F:docs/backlog/archive/PL-240-platform-filesystem-watcher-guidance.md†L1-L27】【F:docs/ROADMAP.md†L64-L101】
- Desired behaviour: Archive completed entries, update roadmap/root README/NAVIGATION links to point at archive locations, and ensure only active work remains in the active directory per maintenance checklist.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L26-L30】【F:README.md†L98-L107】【F:docs/NAVIGATION.md†L96-L107】
- Constraints / invariants: Must preserve citation history and metadata while moving files, respect ADR-0003/ADR-0008 commitments referenced by active tasks, and follow AGENTS deliverable matrix for synchronized documentation updates.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L26-L43】【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L115】【F:AGENTS.md†L32-L117】
- Quality budgets / telemetry notes: Benchmark artefacts and telemetry automation from CC-310/CC-311 must remain accessible post-archival, and PM-510 cadence will broadcast changes; documentation validator must remain clean.【F:docs/backlog/archive/CC-310-comparative-benchmark-automation.md†L26-L48】【F:docs/backlog/archive/CC-311-benchmark-visualisation.md†L26-L48】【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L29】【F:README.md†L122-L142】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Workspace Overview | [`README.md`](../../README.md) | Context ladder instructions and canonical validation commands for doc changes.【F:README.md†L5-L142】 | 1 |
| Navigation Index | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Directory precedence; requires updates when backlog files move.【F:docs/NAVIGATION.md†L1-L113】 | 2 |
| Roadmap | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Lists completed items still referenced from `backlog/active/`; Phase 4 snapshot includes PM-520 as planned.【F:docs/ROADMAP.md†L64-L118】 | 3 |
| Backlog Targets | [`docs/backlog/active/`](../../docs/backlog/active) | Contains completed entries slated for archive (DC-040, DC-041, RT-320, TL-210, RT-321, AS-330, CC-310, CC-311, PL-240).【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L9-L30】 | 4 |
| Module READMEs | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md); [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md); [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) | Cross-checks that active GPU/runtime/tooling work remains in progress and should retain active references.【F:docs/modules/rendering/README.md†L1-L14】【F:docs/modules/runtime/README.md†L1-L36】【F:docs/modules/tools/README.md†L1-L14】 | 5 |
| ADRs | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../docs/specs/ADR-0003-runtime-frame-graph.md); [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Provide binding decisions for GPU/resource and runtime stage planner tasks referenced in roadmap after archival.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L115】 | 6 |
| Historical Audit | [`agents/context_packages/2026-03-01-backlog-roadmap-audit.md`](../context_packages/2026-03-01-backlog-roadmap-audit.md) | Documents previous findings that triggered PM-520; ensures continuity of remediation work.【F:agents/context_packages/2026-03-01-backlog-roadmap-audit.md†L1-L90】 | 7 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Reinforces need to synchronize roadmap/backlog/doc updates per change set and run full validation suite.【F:README.md†L5-L142】 | Product Manager | Copy canonical commands into quality report. |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Confirms navigation updates required when moving backlog files; precedence chain mandates alignment with AGENTS.【F:docs/NAVIGATION.md†L1-L113】 | Knowledge Librarian | Draft navigation edits referencing archive paths. |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 table still links to `backlog/active/` entries; active snapshot lists completed work needing archival.【F:docs/ROADMAP.md†L64-L103】 | Product Manager | Update tables to use archive paths and reflect PL-240 archival note. |
| 4 | [`docs/backlog/active/PM-520-backlog-hygiene-remediation.md`](../../docs/backlog/active/PM-520-backlog-hygiene-remediation.md) | DOD enumerates archival targets, roadmap/nav updates, and validation requirements.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L26-L43】 | Docs/DevRel | Prepare move plan + checklist. |
| 5 | Module READMEs | Rendering/runtime/tools READMEs confirm GPU execution and presentation work remain outstanding (still referencing active backlog IDs).【F:docs/modules/rendering/README.md†L1-L14】【F:docs/modules/runtime/README.md†L1-L36】【F:docs/modules/tools/README.md†L1-L14】 | Specialist Engineer | Ensure cross-links remain after active directory cleanup. |
| 6 | ADRs (`ADR-0003`, `ADR-0008`) | Design contracts emphasise GPU resource provider, command encoder, and runtime stage planner obligations still in progress; documentation must continue to surface them.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L115】 | Chief Architect | No change, but maintain references in roadmap. |
| 7 | Historical audit | Previous context package records audit triggers and ensures our remediation aligns with captured evidence.【F:agents/context_packages/2026-03-01-backlog-roadmap-audit.md†L1-L90】 | Knowledge Librarian | Link this package for continuity. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied: `cmake --preset linux-gcc-debug`, `cmake --build --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, `pytest python/tests scripts/tests`, `python scripts/validate_docs.py`.【F:AGENTS.md†L86-L99】
- Additional presets / datasets: None required beyond default presets; documentation-only change but retain ability to rerun CC-310 smoke presets if telemetry checks become necessary.【F:docs/backlog/archive/CC-310-comparative-benchmark-automation.md†L26-L48】
- Benchmark targets & expected deltas: Expect no performance deltas; ensure comparative benchmark references remain accurate post-archival.【F:docs/backlog/archive/CC-310-comparative-benchmark-automation.md†L26-L48】【F:docs/backlog/archive/CC-311-benchmark-visualisation.md†L26-L48】
- Tooling updates required: Execute documentation validator after moves and update navigation to avoid broken links.【F:docs/backlog/active/PM-520-backlog-hygiene-remediation.md†L27-L30】【F:README.md†L122-L142】

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should archived entries receive additional completion timestamps beyond existing notes? | Docs/DevRel | Before archive commit | Pending (likely retain existing notes only). |
| Are there downstream dashboards referencing `docs/backlog/active/` paths that require manual updates? | Product Manager | Post-implementation review | Pending (monitor after PR). |
| Does roadmap need explicit note that Phase 1–3 items now reside under archive? | Product Manager | During roadmap edit | Pending. |

## 7. Attachments
- Diagrams: None required; architecture unaffected.
- Data sets: N/A (no dataset changes expected).
- Additional notes: Reference prior audit artefacts for historical context and ensure changelog includes archival summary.【F:agents/context_packages/2026-03-01-backlog-roadmap-audit.md†L1-L90】

> **Checklist:** Ensure every link resolves, cite relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
