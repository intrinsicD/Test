# CO-170 Runtime Integration Playbook

**Status:** Draft (foundation complete)  
**Owner:** Compute / Runtime Engineering  
**Updated:** 2025-10-31

## Purpose

Capture the integration contract between the compute kernel dispatcher and
`RuntimeHost`. The playbook documents the `engine_compute_runtime_sample`
executable, telemetry schema, and analysis tooling required to validate
multi-queue orchestration for `CO-170` and upcoming GPU sampling work
(`AN-230`).

## Architecture Overview

```
RuntimeHost::tick
  ├─ animation.evaluate        // animation::advance_controller
  ├─ physics.accumulate        // force aggregation (depends on animation)
  ├─ physics.integrate        // integration & telemetry (depends on accumulate)
  ├─ geometry.deform          // LBS deformation (depends on integrate)
  └─ geometry.finalize        // bounds refresh + scene sync
```

- The dispatcher is instantiated once per tick (CPU backend by default) and
  uses the steady clock configuration from `engine::compute` to capture
  per-kernel durations.
- Runtime diagnostics record stage timings, streaming statistics, and
  subsystem metrics. The sample snapshots stage timings alongside the dispatcher
  report so downstream analysis correlates kernel cost with lifecycle data.

## Sample Workflow

1. **Build the harness**
   ```bash
   cmake --build --preset <preset> --target engine_compute_runtime_sample
   ```
2. **Capture telemetry**
   ```bash
   ./out/build/<preset>/engine/compute/engine_compute_runtime_sample \
       --frames 1024 --dt 0.016 --workload balanced --queues 3 \
       --queue-names main,async-0,async-1 --queue-map physics=async-0 \
       --output telemetry/compute_dispatch.json --pretty
   ```
3. **Analyse results**
   ```bash
   python scripts/diagnostics/compute_dispatch_report.py \
       --input telemetry/compute_dispatch.json --top 5 --jitter-threshold 5
   ```

## Telemetry Payload

The sample emits a JSON document with:

- `metadata` – timestep, dispatcher size, requested frame count, workload
  profile, queue count/names, queue assignments, and clock name/domain.
- `frames[]` – per-frame dispatch order, per-dispatch duration (ms), queue and
  category totals, and aggregate frame time.
- `summary.stage_timings[]` – runtime stage timing snapshot for correlation.
- `summary.queues[]` – queue aggregates mirroring the per-category roll-up.
- `summary.queue_dependencies[]` – aggregated cross-queue fences highlighting
  where timeline synchronization occurs along with the consuming kernels.
- `summary.queue_transitions[]` – edge list capturing every producer/consumer
  pair crossing queue boundaries to help reconstruct fence sequencing.
- `diagnostics` – top-level tick counters and average frame duration.

`compute_dispatch_report.py` recomputes aggregate statistics, highlights
categories and queues with the largest total contribution, and warns when the
standard deviation-to-mean ratio (jitter) exceeds configurable thresholds.
Persist the rendered report (`--output`) in CI to track regressions over time.

## Extending Workloads

- **New kernels:** register additional workloads inside `RuntimeHost::tick`
  before the final `geometry.finalize` kernel. Use the dispatcher dependency
  graph to maintain deterministic ordering.
- **Multiple queues:** map workloads to logical queues via the `--queues`
  command-line flag, rename queues with `--queue-names`, and override per-category
  selection using `--queue-map`. The JSON payload captures both the queue name
  recorded for each dispatch and the resolved `queue_assignments` summary so CI
  reports and dashboards can highlight affinity decisions.
- **GPU backends:** enable CUDA or upcoming compute shader backends via
  dispatcher factory selection to profile GPU execution. Update the playbook
  with backend-specific caveats (synchronisation, staging buffers, telemetry
  adjustments).

## Validation Checklist

- ✅ Sample executes deterministically for seeded workloads (1024 frames @ 16 ms).
- ✅ Telemetry payload parses with `compute_dispatch_report.py` and exposes
  category totals, queue aggregates, and top kernels.
- ✅ Jitter warnings triggered for kernels whose σ/μ exceeds configurable
  thresholds (default 5%).
- 🚧 Follow-up: integrate workload adapters for animation GPU sampling once
  dispatcher queue extensions land.
- 🚧 Follow-up: feed captures into CI dashboards and baseline per-dispatch
  budgets for regression alerts.
