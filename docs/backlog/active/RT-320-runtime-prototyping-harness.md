# Backlog Item RT-320 — Runtime Prototyping Harness

- **Status**: In Progress
- **Priority**: 2
- **Owner**: Runtime Lead
- **Module(s)**: Runtime, Scene, Rendering, Assets, Python Tooling
- **Goal**: Deliver a reusable harness that loads datasets, toggles algorithms, and records telemetry for AI-004 case studies.

## Summary
Researchers currently assemble ad-hoc executables to validate algorithms. RT-320 establishes a supported harness built on `RuntimeHost` with both interactive and headless flows. It consumes the shared AI-004 configuration schema, exposes scripting hooks, and produces telemetry artefacts compatible with benchmarking automation.

## Definition of Done
- [ ] Harness boots from a schema-driven configuration, loads reference datasets, and configures the research rendering baseline.
- [ ] Interactive mode offers camera/navigation controls, overlay toggles, and telemetry capture integrated with the tools sandbox.
- [ ] Headless mode runs scripted scenarios and writes reproducible telemetry outputs.
- [ ] Runtime documentation and samples include setup instructions plus example configuration files.

## Dependencies
- [`docs/backlog/active/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)
- Research rendering baseline (see archived `RE-610`).
- Runtime integration sample (`CO-170`) for orchestration patterns.

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/RT-320-runtime-prototyping-harness.md`](../../archive/backlog/legacy/tasks/RT-320-runtime-prototyping-harness.md)
- [`docs/design/RT-320-prototyping-harness.md`](../../design/RT-320-prototyping-harness.md) *(create if missing during implementation)*

## Notes
This item unblocks TL-210, CC-310, and RT-321. Coordinate daily with rendering/tools leads during implementation.

**2026-02-05** — Native `runtime_prototype_harness` sample added under `engine/runtime/samples/`. Provides schema-backed CLI,
dry-run summary generation, and CTest coverage aligned with the Python scaffold.
