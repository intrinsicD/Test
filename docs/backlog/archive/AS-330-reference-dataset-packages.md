# Backlog Item AS-330 — Reference Dataset Packages

- **Status**: Complete
- **Priority**: 3
- **Owner**: Assets Lead
- **Module(s)**: Assets, Geometry, Rendering, Python Tooling
- **Goal**: Provide curated, licensed datasets with manifests and ingestion tooling that plug into the AI-004 workflow.

## Summary
AI-004 requires canonical datasets so harness runs are reproducible and comparable. AS-330 selects permissively licensed assets, packages them with manifests and ingestion scripts, and wires them into the runtime harness and sandbox UI. Each package documents provenance, recommended scenarios, and validation checks to guarantee consistent scale and materials.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Coordinate licensing reviews, ingestion automation, and downstream consumers. | Agent Orchestrator (11) |
| Product Manager | Ensure dataset scope meets roadmap milestones and legal requirements. | Product Manager (10) |
| Knowledge Librarian | Archive provenance, licensing notes, and ingestion documentation across modules. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Assets lead builds packages; geometry/rendering engineers verify fidelity; Python tooling owner wires ingestion. | Assets Lead (primary); Geometry & Rendering specialists; Python tooling owner |
| Docs/DevRel | Document dataset installation, troubleshooting, and reproducibility checklists. | Docs/DevRel (95) |
| QA/Test Specialist | Automate ingestion/regression tests; capture checksum outputs. | QA/Test Specialist (90) |
| Performance Engineer | Measure streaming performance and storage impact for CI/runtime usage. | Performance Engineer (80) |
| Safety Reviewer | Validate licensing compliance, data sanitisation, and distribution restrictions. | Safety Reviewer (15) |
| Reviewer | Review manifests/scripts for adherence to standards. | Reviewer (99) |
| Release Manager | Publish dataset bundles and update release notes with installation guidance. | Release Manager (98) |

## Definition of Done
- [x] Publish dataset manifests covering at least geometry, rendering, and animation categories with provenance and licensing notes.
- [x] Automated ingestion script downloads, validates (checksums), and registers datasets with engine asset caches.
- [x] Runtime harness and sandbox UI list available datasets via the shared configuration schema.
- [x] Assets module README and prototyping playbook document installation, troubleshooting, and reproducibility checklist.

## Dependencies
- [`docs/backlog/archive/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)
- [`docs/backlog/archive/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/AS-330-reference-dataset-packages.md`](../../archive/backlog/legacy/tasks/AS-330-reference-dataset-packages.md)
- [`scripts/diagnostics/streaming_report.py`](../../../scripts/diagnostics/streaming_report.py) for ingest validation

## Notes
Ensure licensing review completes before publishing manifests. Provide smaller subsets for CI usage to keep runtimes deterministic.

**2026-02-13** — Remesh sample manifest now records split/collapse counts, run duration,
triangle totals, and triangle quality metrics. The ingestion summary exports these
fields to simplify telemetry comparisons in harness describe outputs and benchmark
automation.
**2026-02-14** — Rendering and animation reference manifests added under
`assets/datasets/rendering_sample` and `assets/datasets/animation_sample` with
schema v2 provenance/licensing metadata. Assets README and the prototyping
playbook document ingestion commands while the examples index links the new
dataset slugs for harness selectors.

**2026-02-15** — `docs/examples/ai004_sample.json` now imports the rendering and
animation dataset manifests so the harness `--describe-json` export and sandbox
dataset browser enumerate all packaged slugs (`remesh-unit-square`,
`rendering-quad-shading`, `animation-walk-retarget`) directly from the shared
configuration schema.
**2026-02-18** — README, roadmap, and module TODO updates mark AS-330 complete and highlight the upcoming licensing refresh planning cycle.
