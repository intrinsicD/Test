# MASTER AGENT PROMPT

Purpose: Operate end-to-end as a coding agent in this repository. Follow the phases defined in [`AGENT_WORKFLOW.md`](../../AGENT_WORKFLOW.md), select the highest-value task, plan it, implement it, verify with quality gates, self-review, and deliver cleanly. Work autonomously; only ask for input if truly blocked by missing facts in the repo.

How to call this prompt
- Chat: “Load and execute docs/prompts/MASTER-AGENT-PROMPT.md. Start at Section 1 (Task discovery), produce acceptance criteria, then proceed through Sections 2–7 to fully implement and verify the task.”
- Terminal (copy to clipboard):
  - xclip (X11): `xclip -selection clipboard < docs/prompts/MASTER-AGENT-PROMPT.md`
  - wl-copy (Wayland): `wl-copy < docs/prompts/MASTER-AGENT-PROMPT.md`
  - Or just view and copy: `cat docs/prompts/MASTER-AGENT-PROMPT.md`

Key playbooks to use:
- docs/prompts/IMPLEMENTATION-PLAYBOOK.md — implementing features/bug fixes.
- docs/prompts/REFACTOR-PLAYBOOK.md — code health/structural changes without new behavior.
- docs/prompts/REVIEW-CHECKLIST.md — self-review after implementation.
- docs/prompts/ARCHITECTURE-AUDIT.md — for cross-cutting or boundary changes.

Guardrails:
- Prefer minimal, reversible diffs. Keep commits focused. Avoid scope creep.
- Do not exfiltrate secrets or rely on network calls unless explicitly required here.
- Don’t wait for the user; pull all needed context from the repo. Note assumptions when made.
- Always run quality gates and ensure green before declaring done.

Workflow alignment checklist:
- Load [`../../AGENT_WORKFLOW.md`](../../AGENT_WORKFLOW.md) and map Sections 1–7 below to its phases (Intake & Scoping → §1, Context Assembly → §2, Execution & Collaboration → §3, Quality Gates → §4, Release & Documentation Sync → §§5–7).
- Inspect the backlog entry’s **Role Roster** (`docs/backlog/active/<ID>.md`) and confirm each accountable role has a plan before exiting Section 2.
- Update the task brief/context package templates in [`../../agents/TEMPLATES/`](../../agents/TEMPLATES) whenever scope, roster, or status shifts.

---

## Execution rules (non‑negotiable)
- After Section 1 (Task discovery), you must continue through Sections 2–7 and actually modify files in this repository to implement the selected task.
- Do not stop after planning or acceptance criteria. Make concrete edits, add/update tests, and run builds/tests locally.
- Prefer direct file edits in this workspace. If you cannot edit files, output minimal unified diffs the user can apply and the exact commands to run.
- Do not simulate results. Report real build/test outcomes (summaries are fine) and iterate until green.
- Only stop when the Stop criteria are satisfied.

---

## 1) Task discovery and selection *(Workflow Phase: Intake & Scoping)*
Read and synthesize from these local sources to find work:
- docs/ROADMAP.md, docs/NAVIGATION.md (priorities/scope)
- docs/backlog/active/ (focus on Role Roster + Definition of Done), docs/specs/, docs/reviews/ (if present)
- TODO/FIXME in code (search repo)
- Failing tests (run the test suites)

Shortlist 2–4 candidate tasks and rank by value, urgency, risk, and dependency availability. Pick one task.

Write acceptance criteria (3–5 bullets). Capture how each accountable role from the Role Roster contributes to meeting those criteria. If criteria are unclear, make the smallest reasonable assumptions and record them.

## 2) Planning — small contract *(Workflow Phase: Context Assembly)*
Before coding, define a short plan that states:
- Inputs/outputs, affected APIs/types, data shapes, side effects
- Files/modules you’ll touch; any build system changes (CMake)
- Tests to add/modify (unit/integration) and 2–4 edge cases to cover
- Success criteria tied to the acceptance bullets
- Which roles require sign-off or parallel work (Docs/DevRel, QA/Test Specialist, Performance Engineer, etc.) and how you will coordinate with them

Choose the minimal viable path to green that leaves room to extend later.

## 3) Implementation *(Workflow Phase: Execution & Collaboration)*
- If it’s a feature/bug fix: follow docs/prompts/IMPLEMENTATION-PLAYBOOK.md
- If it’s primarily refactor: follow docs/prompts/REFACTOR-PLAYBOOK.md
- Update or add tests alongside code. Keep changes cohesive and readable.
- Make real edits in this workspace; do not merely describe hypothetical changes.
- Capture coordination notes for each active role in the task brief/context package.

## 4) Quality gates (must pass) *(Workflow Phase: Quality Gates)*
C++ (CMake):
- If presets are configured, list them and use appropriate ones:
  - cmake --list-presets
  - cmake --preset <DebugPresetName>
  - cmake --build --preset <DebugPresetName>
  - ctest --preset <DebugPresetName>
  - cmake --preset <ReleasePresetName>
  - cmake --build --preset <ReleasePresetName>
  - ctest --preset <ReleasePresetName>
- If presets aren’t usable, fallback:
  - cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
  - cmake --build cmake-build-debug -j
  - ctest --test-dir cmake-build-debug
  - cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
  - cmake --build cmake-build-release -j
  - ctest --test-dir cmake-build-release

Python (only if Python code changed or tests need to run):
- python -m venv .venv
- source .venv/bin/activate
- pip install -r python/requirements.txt
- pytest python/tests scripts/tests

Lint/format (if configured):
- Prefer existing scripts in scripts/lint or project config. Keep diffs clean.

Do a small smoke test if the change is user-visible. Record evidence in the quality report template owned by the QA/Test Specialist.

## 5) Documentation and self-review *(Workflow Phase: Release & Documentation Sync — Docs/DevRel focus)*
- Update relevant docs (docs/modules/, READMEs, code comments) if behavior, APIs, or usage changed.
- Run docs/prompts/REVIEW-CHECKLIST.md on your diff; fix issues you find.
- If the change crosses module boundaries or affects architecture, also run docs/prompts/ARCHITECTURE-AUDIT.md and address findings.
- Ensure documentation updates satisfy the backlog item’s Role Roster expectations for Docs/DevRel and Knowledge Librarian.

## 6) Delivery *(Workflow Phase: Release & Documentation Sync — Release Manager focus)*
- Commit message:
  - Title (imperative, scoped): e.g., core: fix X by Y
  - Body: problem, approach, alternatives considered, risks/limitations, tests added/updated, docs updates
- Note any follow-ups or deferred items with rationale, and capture role sign-off status in the task brief.

## 7) Stop criteria *(Workflow Phase: Release & Documentation Sync completion)*
Stop only when ALL are true:
- Acceptance criteria are met
- You have modified the repository (code and/or tests) to implement the task
- CMake builds pass (Debug and Release) and tests are green (report outcomes)
- Lint/format checks pass (if configured)
- Review checklist(s) are satisfied
- Relevant docs/comments are updated (remove stale guidance if any)
- Role Roster owners have signed off or have tracked follow-ups

---

## Helpful command scaffolds (adapt as needed)
CMake with presets:
- cmake --list-presets
- cmake --preset=<name>
- cmake --build --preset=<name>
- ctest --preset=<name>

CMake without presets:
- cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
- cmake --build cmake-build-debug -j
- ctest --test-dir cmake-build-debug
- cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
- cmake --build cmake-build-release -j
- ctest --test-dir cmake-build-release

Python:
- python -m venv .venv
- source .venv/bin/activate
- pip install -r python/requirements.txt
- pytest python/tests scripts/tests

---

Operating mode: Act autonomously. Make concrete edits and run the project locally. Do not prompt the user unless blocked by missing facts that cannot be inferred from the codebase and docs. Prefer concrete edits, tests, and verification over high-level suggestions. Do not stop after acceptance criteria. Keep workflow notes synchronized with [`agents/TEMPLATES/`](../../agents/TEMPLATES) deliverables.
