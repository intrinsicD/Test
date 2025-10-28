# Backlog Item RT-321 — Prototyping Case Study Validation

- **Status**: Planned
- **Priority**: 3
- **Owner**: Runtime Lead (with Assets & Tools support)
- **Module(s)**: Runtime, Assets, Rendering, Tools, Python
- **Goal**: Demonstrate the harness across two reproducible case studies that capture telemetry and benchmark artefacts.

## Summary
After RT-320 delivers the harness we must prove it supports real workflows. RT-321 curates two case studies—one geometry-heavy, one rendering-heavy—that run end to end through the AI-004 configuration schema. Each scenario loads packaged datasets, configures the research baseline, exports telemetry compatible with CC-310, and is callable from both the CLI and the sandbox UI.

## Definition of Done
- [ ] Case study manifests live under `assets/datasets/` and are referenced by harness CLI/SDK helpers.
- [ ] Harness CLI exposes `--case-study` presets that configure datasets, algorithms, and telemetry sinks.
- [ ] Sandbox UI enumerates case studies with default parameter sets and benchmark capture toggles.
- [ ] Integration + CI coverage executes both scenarios and records baseline metrics in documentation.

## Dependencies
- [`docs/backlog/active/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)
- [`docs/backlog/active/AS-330-reference-dataset-packages.md`](AS-330-reference-dataset-packages.md)
- [`docs/backlog/active/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/RT-321-prototyping-case-study-validation.md`](../../archive/backlog/legacy/tasks/RT-321-prototyping-case-study-validation.md)
- To document baselines: extend [`docs/design/RT-320-prototyping-harness.md`](../../design/RT-320-prototyping-harness.md) or author `RT-321-case-studies.md`.

## Notes
Case study telemetry feeds CC-310/CC-311. Coordinate dataset licensing early to avoid blocking integration tests.
