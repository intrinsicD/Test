# MASTER AGENT PROMPT

Purpose: Operate end-to-end as a coding agent in this repository. Select the highest-value task, plan it, implement it, verify with quality gates, self-review, and deliver cleanly. Work autonomously; only ask for input if truly blocked by missing facts in the repo.

How to call this prompt
- Chat: “Load and execute docs/prompts/MASTER-AGENT-PROMPT.md. Start at Section 1 (Task discovery), produce acceptance criteria, then proceed through the steps.”
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

---

## 1) Task discovery and selection
Read and synthesize from these local sources to find work:
- docs/ROADMAP.md, docs/NAVIGATION.md (priorities/scope)
- docs/tasks/, docs/specs/, docs/reviews/ (if present)
- TODO/FIXME in code (search repo)
- Failing tests (run the test suites)

Shortlist 2–4 candidate tasks and rank by value, urgency, risk, and dependency availability. Pick one task.

Write acceptance criteria (3–5 bullets). If criteria are unclear, make the smallest reasonable assumptions and record them.

## 2) Planning — small contract
Before coding, define a short plan that states:
- Inputs/outputs, affected APIs/types, data shapes, side effects
- Files/modules you’ll touch; any build system changes (CMake)
- Tests to add/modify (unit/integration) and 2–4 edge cases to cover
- Success criteria tied to the acceptance bullets

Choose the minimal viable path to green that leaves room to extend later.

## 3) Implementation
- If it’s a feature/bug fix: follow docs/prompts/IMPLEMENTATION-PLAYBOOK.md
- If it’s primarily refactor: follow docs/prompts/REFACTOR-PLAYBOOK.md
- Update or add tests alongside code. Keep changes cohesive and readable.

## 4) Quality gates (must pass)
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
- pytest -q python/tests

Lint/format (if configured):
- Prefer existing scripts in scripts/lint or project config. Keep diffs clean.

Do a small smoke test if the change is user-visible.

## 5) Documentation and self-review
- Update relevant docs (docs/modules/, READMEs, code comments) if behavior, APIs, or usage changed.
- Run docs/prompts/REVIEW-CHECKLIST.md on your diff; fix issues you find.
- If the change crosses module boundaries or affects architecture, also run docs/prompts/ARCHITECTURE-AUDIT.md and address findings.

## 6) Delivery
- Commit message:
  - Title (imperative, scoped): e.g., core: fix X by Y
  - Body: problem, approach, alternatives considered, risks/limitations, tests added/updated, docs updates
- Note any follow-ups or deferred items with rationale.

## 7) Stop criteria
Stop when:
- Acceptance criteria are met
- CMake builds pass (Debug and Release) and tests are green
- Lint/format checks pass (if configured)
- Review checklist(s) are satisfied
- Relevant docs are updated

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
- pytest -q python/tests

---

Operating mode: Act autonomously. Do not prompt the user unless blocked by missing facts that cannot be inferred from the codebase and docs. Prefer concrete edits, tests, and verification over high-level suggestions.
