# AGENTS — Single Entry Point

**Mission.** Ship the highest-priority AI-004 task safely, fast, and to spec while keeping docs/backlog/roadmap in sync.

## Lifecycle
`new → ready → in_progress → review → done → archived`

## Workflow (follow exactly)

1) **Select Task**
   - Start with [`docs/ROADMAP.md`](../docs/ROADMAP.md) and pick the highest-priority item marked `status: ready`.
   - If nothing is ready, groom the top `new` backlog item under `docs/backlog/active/` and mark it `ready` once scoped.

2) **Plan & Design**
   - Open the task file (e.g. `docs/backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md`).
   - Complete the **Design/Plan** section referencing constraints from [`workflow/CONTRIBUTING.md`](./CONTRIBUTING.md) and relevant module READMEs.
   - Capture context via task brief + context package templates in `agents/TEMPLATES/`; link ADRs when architecture shifts.

3) **Implement**
   - Branch name: `feat|fix|refactor/NNN-kebab-title` (matches backlog ID).
   - Follow coding standards, keep commits reviewable (<400 LOC), and add Tracy zones to profiled hot paths.
   - Update docs, presets, and datasets alongside code when behaviour changes.

4) **Test**
   - Run the canonical stack:
     - `cmake --preset linux-gcc-debug`
     - `cmake --build --preset linux-gcc-debug`
     - `ctest --preset linux-gcc-debug`
     - `pytest python/tests scripts/tests`
     - `python scripts/validate_docs.py`
   - Capture outputs in the quality report with benchmark deltas and telemetry snapshots when required.

5) **Review**
   - Open a PR referencing the backlog file path.
   - Attach task brief, context package, and quality report links; flag outstanding risks/blockers.

6) **Complete**
   - Set `status: done` in the backlog file and archive it under `docs/backlog/archive/`.
   - Update `docs/ROADMAP.md` checkboxes and refresh README/module snapshots as needed.
   - Commit artefact updates (task brief, context package, quality report) and run `python scripts/update_agents_tree.py` if structure changed.

7) **Docs Sync**
   - Validate cross-links via `python scripts/validate_docs.py`.
   - Ensure navigation tables (`docs/NAVIGATION.md`) and module READMEs reflect new assets or behaviour.

## Blockers
- Add `blocked_on:` entries inside the backlog file when dependencies arise and notify the Agent Orchestrator.
- Pause implementation, pick the next ready task, and document the hand-off in the task brief communication log.

## Quality Gates
- Tests/linters green; perf regressions ≤2% unless mitigation accepted.
- Documentation updated (README, module guides, roadmap, backlog) within the same change.
- Safety, perf, or docs gates listed in backlog front matter must be explicitly signed off.

> All work must trace back to a file under `docs/backlog/`. Keep artefacts, documentation, and automation outputs synchronized in every commit.
