# Physics Module Roadmap

_Last Updated: 2025-03-25_

## Key Initiatives

| Initiative | Description | Status |
| --- | --- | --- |
| Persistent manifolds (`PH-401`) | Maintain contact data across frames to improve stability. | ✅ Done |
| Benchmarking (`PH-410`) | Establish throughput benchmarks for collisions. | ✅ Done |
| Telemetry (`PH-420`) | Expose manifold churn and solver metrics. | ✅ Done |

## Active Task

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| – | – | – | ✅ Completed (awaiting next scope) |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| – | Define next diagnostics automation milestone following `PH-430`. | Pending planning |

## Notes

- Coordinate with runtime to surface manifold telemetry and benchmark trends for diagnostics viewers.
- Capture benchmark results in task record `T-0117-physics-contact-manifolds.md` and update CI baselines as they evolve.
- Telemetry schema (`CC-001.1`) is available; runtime now publishes manifold
  metrics under the shared schema, completing `PH-420`.
- Collision throughput harness (`engine_physics_benchmarks`) emits JSON metrics
  for CI ingestion; `PH-430` now exposes those artefacts via
  `scripts/diagnostics/collision_benchmark_report.py`.
