# Physics Module Roadmap

_Last Updated: 2025-02-19_

## Key Initiatives

| Initiative | Description | Status |
| --- | --- | --- |
| Persistent manifolds (`PH-401`) | Maintain contact data across frames to improve stability. | 🔄 In Progress |
| Benchmarking (`PH-410`) | Establish throughput benchmarks for collisions. | 🟢 Planned |
| Telemetry (`PH-420`) | Expose manifold churn and solver metrics. | ✅ Done |

## Active Task

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| `PH-401` | Physics team | 2025-03-21 | 🔄 In Progress |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `PH-410` | Build benchmarking harness integrated with CI. | After `PH-401` |
| `PH-420` | Emit telemetry and document diagnostics workflow. | Requires `CC-001.1` schema |

## Notes

- Coordinate with runtime to surface manifold telemetry.
- Capture benchmark results in task record `T-0117-physics-contact-manifolds.md`.
- Telemetry schema (`CC-001.1`) is available; runtime now publishes manifold
  metrics under the shared schema, completing `PH-420`.
