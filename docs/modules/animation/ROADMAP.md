# Animation Module Roadmap

_Last Updated: 2025-02-19_

## Milestone Phases

| Phase | Scope | Status |
| --- | --- | --- |
| Phase 1 – Foundation | Clip validation, JSON serialization, controller evaluation. | ✅ Complete |
| Phase 2 – Integration | Validation hardening, deformation alignment, telemetry hooks. | 🔄 In Progress |
| Phase 3 – Advanced Features | GPU/parallel sampling, state-machine authoring, advanced deformation. | 🟢 Planned |

## Active Work (Phase 2)

| Task ID | Description | Owner | Due | Status |
| --- | --- | --- | --- | --- |
| `AN-201` | Extend regression coverage for `validate_clip` failure cases and controller validation. | Animation team | 2025-03-07 | 🔄 In Progress |
| `AN-220` | Finalise deformation binding docs linked to `RT-001` outcomes. | Animation team | 2025-03-14 | ✅ Done |
| `AN-225` | Mirror animation diagnostics into runtime telemetry dashboards. | Animation + Runtime | 2025-03-21 | 🟢 Todo |

## Upcoming (Phase 3)

| Task ID | Description | Trigger |
| --- | --- | --- |
| `AN-230` | Prototype GPU/parallel sampling and benchmark controller throughput with `compute::KernelDispatcher`. | After `CO-141` lands. |
| `AN-240` | Draft state-machine authoring specification with transition orchestration and event propagation requirements. | After `AN-201` complete. |
| `AN-250` | Investigate advanced deformation pipelines (dual quaternion, curve-driven rigs) and dependencies on geometry module upgrades. | Pending geometry remeshing roadmap. |

## Dependencies

- **Geometry module** — provides mesh access for deformation pipelines.
- **Runtime module** — consumes animation poses and telemetry.
- **Compute module** — supplies dispatcher for GPU/parallel sampling experiments.

## Notes

- Record benchmark data in [`docs/tasks/T-0113-animation-runtime-skinning.md`](../../tasks/T-0113-animation-runtime-skinning.md).
- Align telemetry hooks with the broader diagnostics initiative (`CC-001`).
