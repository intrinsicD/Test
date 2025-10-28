# Backlog Item RE-610 — Research Rendering Baseline

- **Status**: Complete (Archived)
- **Priority**: 2
- **Owner**: Rendering Lead
- **Module(s)**: Rendering, Tools, Runtime
- **Goal**: Ship a research-grade rendering preset with telemetry hooks for AI-004 workflows.

## Summary
RE-610 delivered forward and deferred shading presets with debug overlays, configuration toggles, and telemetry instrumentation. The preset now underpins the runtime prototyping harness and sandbox UI. This archive captures the outcome for historical reference.

## Definition of Done (Achieved)
- [x] Rendering baseline renders reference scenes ≥120 FPS with forward/deferred modes.
- [x] Debug overlays for normals, UVs, material layers, and light volumes.
- [x] Telemetry metrics (per-pass timing, draw counts, shading mode) exported to runtime diagnostics.
- [x] Documentation updated in rendering module README and prototyping playbook.

## Dependencies
- Built on AI-001 through AI-003 groundwork.

## Related Artefacts
- Legacy detail: [`docs/archive/backlog/legacy/tasks/RE-610-research-rendering-baseline.md`](../../archive/backlog/legacy/tasks/RE-610-research-rendering-baseline.md)
- Implementation: `engine/rendering/src/pipeline/research_baseline.cpp`
- Sample: `engine/tools/examples/geometry_viewer.cpp`

## Notes
Keep this preset updated as new telemetry channels or overlays are introduced; future work will extend post-processing as part of AI-004 follow-ups.
