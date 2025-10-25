# Animation Module Roadmap

_Last Updated: 2025-11-19_

## Milestone Phases

| Phase | Scope | Status |
| --- | --- | --- |
| Phase 1 – Foundation | Clip validation, JSON serialization, controller evaluation. | ✅ Complete |
| Phase 2 – Integration | Validation hardening, deformation alignment, telemetry hooks. | 🔄 In Progress |
| Phase 3 – Advanced Features | GPU/parallel sampling, state-machine authoring, advanced deformation. | 🟢 Active |

## Active Work (Phase 2)

| Task ID | Description | Owner | Due | Status |
| --- | --- | --- | --- | --- |
| `AN-201` | Extend regression coverage for `validate_clip` failure cases and controller validation. | Animation team | 2025-03-07 | ✅ Done |
| `AN-220` | Finalise deformation binding docs linked to `RT-001` outcomes. | Animation team | 2025-03-14 | ✅ Done |
| `AN-225` | Mirror animation diagnostics into runtime telemetry dashboards. | Animation + Runtime | 2025-03-21 | 🟢 Todo |

## Active Work (Phase 3)

| Task ID | Description | Owner | Due | Status |
| --- | --- | --- | --- | --- |
| `AN-230` | Prototype GPU/parallel sampling and benchmark controller throughput with `compute::KernelDispatcher` ([plan](../../design/ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md)). | Animation + Compute | 2025-12-05 | ✅ Done |

## Upcoming (Phase 3)

| Task ID | Description | Trigger |
| --- | --- | --- |
| `AN-240` | Draft state-machine authoring specification with transition orchestration and event propagation requirements ([spec](../../specs/AN-240-state-machine-authoring.md)). | After `AN-201` complete. |
| `AN-250` | Investigate advanced deformation pipelines (dual quaternion, curve-driven rigs) and dependencies on geometry module upgrades. | Pending geometry remeshing roadmap. |

## Dependencies

- **Geometry module** — provides mesh access for deformation pipelines.
- **Runtime module** — consumes animation poses and telemetry.
- **Compute module** — supplies dispatcher for GPU/parallel sampling experiments.

## Notes

- Record benchmark data in [`docs/tasks/T-0113-animation-runtime-skinning.md`](../../tasks/T-0113-animation-runtime-skinning.md).
- Align telemetry hooks with the broader diagnostics initiative (`CC-001`).
- 2025-11-19: Published `animation_sampling_report.py` to summarise CPU/GPU captures and closed `AN-230.3`, completing the benchmarking milestone.
- 2025-11-18: Benchmark driver exposes `gpu_async` scenario via `compute::Dispatcher`, emitting queue/category telemetry compatible with the dispatcher schema (`AN-230.2`).
- 2025-11-16: `engine_animation_benchmark_driver` added to capture CPU baselines aligned with dispatcher telemetry (`AN-230.1`).
- 2025-11-11: Activated `AN-230` benchmarking after `CO-170` completion; coordination with compute/runtime documented in the central roadmap.
- 2025-03-24: Logged GPU sampling benchmark plan (`AN-230`) and state-machine
  authoring specification (`AN-240`) under `docs/design/` and `docs/specs/`.
- 2025-02-21: Added regression coverage for clip validation failure codes and
  controller playback invariants (`engine/animation/tests/test_clip_serialization.cpp`,
  `engine/animation/tests/test_module.cpp`).
