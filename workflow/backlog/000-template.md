````markdown
---
id: NNN
title: Short imperative title
status: new            # new | ready | in_progress | review | done | archived
priority: P1           # P0 | P1 | P2
area: rendering        # rendering | geometry | runtime | tools | docs | infra | ...
size: M                # XS | S | M | L
owner: agent           # or a handle
gates: [tests]         # add: perf | docs | safety | release
relates_to: [bundle:A] # bundle tag from ROADMAP
blocked_on: []         # ["dep or reason"]
links: []              # PRs, ADRs, docs
---

## Intent
<!-- MIGRATE: one sentence outcome/value -->

## Design / Plan
<!-- MIGRATE: constraints, API sketch, data layout, edge cases -->
- Constraints:
  - Follow `workflow/CONTRIBUTING.md` plus module README guardrails.
  - Maintain determinism, data-oriented layouts (SoA) in hot paths, and documented telemetry budgets.
  - Keep documentation/backlog/roadmap entries in sync with behaviour.
- API/data sketch:
```cpp
// (Optional) your API/data structure plan
````

* Edge cases & failure modes:

  * {{case 1}}
  * {{case 2}}
* Test plan:

  * Unit: {{what proves correctness}}
  * Perf (if gate set): dataset + target (e.g., ≥2×, p95 < X ms)
  * Regression: metric that future PRs must not degrade

## Steps

1. {{step}}
2. {{step}}
3. {{step}}

## Evidence

* Unit tests: `engine/<module>/tests/...`
* Python/tests: `python/tests/...` or `scripts/tests/...`
* Benchmarks: `engine/<module>/bench/...` or telemetry captures under `telemetry/`
* Artifacts/logs: {{links or file paths}}

<!-- Agent: paste key numbers, screenshots, and SHA links here -->

## Completion Checklist (DoD)

* [ ] Tests added & green (cmake preset + pytest + docs validator)
* [ ] Perf targets met & recorded (if gate)
* [ ] Docs updated if user/API facing
* [ ] ROADMAP item checked
* [ ] Cross-links validated (`python scripts/validate_docs.py`)

## Result

* PR: {{link}} (SHA: {{hash}})
* Notes: {{anything notable for future readers}}

````
