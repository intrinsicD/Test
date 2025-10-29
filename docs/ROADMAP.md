# Central Roadmap

This roadmap defines how the engine reaches **application readiness**—a state where research teams can prototype, benchmark, and ship interactive experiences with confidence. It complements module READMEs and the backlog entries under [`docs/backlog/`](backlog/). Update all three whenever priorities shift.

## Vision

> Deliver a production-capable runtime and tooling stack that loads curated datasets, renders with high fidelity, and measures performance against reference implementations.

Key outcomes:
This plan tracks the `AI-004` Application Prototyping Enablement initiative and breaks it into phased milestones.
- Researchers launch experiments from a shared configuration schema without ad-hoc wiring.
- Interactive and headless workflows share the same datasets, presets, and telemetry.
- Comparative benchmarks run automatically in CI and surface regressions within two percent.

## Priority Bands

Priorities use a numeric scale (**1 = highest urgency**, **5 = lowest**). Each milestone below cites backlog items and their current priority so agents can jump directly to the owning file.

## Path to Application Readiness

### Phase 0 — Foundations *(Complete)*
- ✅ [`RE-610`](backlog/archive/RE-610-research-rendering-baseline.md) *(Priority 2)* — Research rendering baseline with telemetry overlays.
- ✅ AI-001 to AI-003 infrastructure (handles, streaming, frame-graph metadata).

### Phase 1 — Kickoff Ready *(Priorities 1–2)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 1 | [`DC-040`](backlog/active/DC-040-ai-004-configuration-schema.md) | Ratify shared configuration schema and validators. | Agent Orchestrator + Module Leads | In Progress |
| 1 | [`DC-041`](backlog/active/DC-041-ai-004-kickoff-readiness.md) | Publish kickoff plan, risk ownership, and milestone sequencing. | Product Manager | Planned |
| 2 | [`RT-320`](backlog/active/RT-320-runtime-prototyping-harness.md) | Ship schema-driven harness with interactive + headless flows. | Runtime Lead | Planned |

**Exit Criteria:** Schema approved, kickoff brief published, harness prototype booting sample datasets.

### Phase 2 — Harness & Dataset Integration *(Priorities 2–3)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 3 | [`AS-330`](backlog/active/AS-330-reference-dataset-packages.md) | Curate licensed datasets with manifests and ingestion tooling. | Assets Lead | Planned |
| 3 | [`TL-210`](backlog/active/TL-210-experiment-sandbox-ui.md) | Finalise sandbox UI wiring to harness + benchmarks. | Tools Lead | In Progress |
| 3 | [`RT-321`](backlog/active/RT-321-prototyping-case-studies.md) | Validate two case studies end to end with telemetry baselines. | Runtime Lead | Planned |

**Exit Criteria:** Harness, sandbox, and datasets operate together; case studies produce reproducible artefacts.

### Phase 3 — Benchmark Confidence *(Priorities 4–5)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 4 | [`CC-310`](backlog/active/CC-310-comparative-benchmark-automation.md) | Automate comparative benchmarks and CI smoke suite. | Performance Lead | Planned |
| 4 | [`CC-311`](backlog/active/CC-311-benchmark-visualisation.md) | Render comparative reports and expose them in tooling + CI. | Performance Lead | Planned |
| 5 | Follow-up polish items (to be spawned once CC-311 closes). | — | — |

**Exit Criteria:** Automated benchmarks and visual reports block regressions; kickoff demo is reproducible on CI hardware.

## Active Backlog Snapshot

| Backlog | Priority | Status | Notes |
| --- | --- | --- | --- |
| [`DC-040`](backlog/active/DC-040-ai-004-configuration-schema.md) | 1 | In Progress | Schema ADR drafted; Python + native runtime validators landed. |
| [`DC-041`](backlog/active/DC-041-ai-004-kickoff-readiness.md) | 1 | Planned | Awaits schema sign-off. |
| [`RT-320`](backlog/active/RT-320-runtime-prototyping-harness.md) | 2 | Planned | Blocks TL-210/RT-321/CC-310. |
| [`AS-330`](backlog/active/AS-330-reference-dataset-packages.md) | 3 | Planned | Licensing review pending. |
| [`TL-210`](backlog/active/TL-210-experiment-sandbox-ui.md) | 3 | In Progress | UI scaffolding merged; harness wiring outstanding. |
| [`RT-321`](backlog/active/RT-321-prototyping-case-studies.md) | 3 | Planned | Depends on datasets + harness. |
| [`CC-310`](backlog/active/CC-310-comparative-benchmark-automation.md) | 4 | Planned | Draft orchestrator script landed; CI wiring TBD. |
| [`CC-311`](backlog/active/CC-311-benchmark-visualisation.md) | 4 | Planned | Requires CC-310 outputs + case study baselines. |

## Risks & Mitigations

| Priority | Risk | Owner | Mitigation |
| --- | --- | --- | --- |
| 1 | Schema consensus slips, delaying harness integration. | Agent Orchestrator | Timebox reviews; escalate to Chief Architect after 2 business days. |
| 2 | Harness complexity outpaces test coverage. | Runtime Lead | Land integration smoke tests before expanding feature scope. |
| 3 | Dataset licensing blocks case study publication. | Assets Lead | Finalise licensing shortlist early; provide fallback synthetic datasets. |
| 4 | Benchmark automation exceeds CI time budget. | Performance Lead | Maintain smoke preset (<5 min) separate from nightly full suite. |

## Maintenance Checklist

- Review this roadmap weekly; update status/progress before each sync.
- When a backlog item completes, move its file to [`backlog/archive/`](backlog/archive/) and update tables above.
- Keep module READMEs and the root [`README.md`](../README.md) aligned with the priority bands listed here.
- Run `python scripts/validate_docs.py` after editing roadmap or backlog files to catch broken links.

**Last updated:** 2026-01-?? (Set automatically when committing).
