# AN-230.2 GPU Sampling Telemetry Integration

**Status:** Draft → Implemented in this change  
**Owner:** Animation/Compute Tech Lead  
**Related Tasks:** `AN-230.2` (GPU sampling kernels with telemetry validation)  
**Depends On:** `AN-230.1` harness baseline, `CO-170` dispatcher telemetry workflow

## 1. Goals & Constraints

We need to extend `engine_animation_benchmark_driver` so that it can exercise a
GPU sampling path while emitting telemetry compatible with the dispatcher
report schema introduced by `CO-170`. The integration must:

- Preserve the CPU baseline flow while adding a selectable GPU scenario.
- Route work through `compute::Dispatcher`, tagging queue assignments and
  per-dispatch timings so downstream tooling treats the output like any other
  runtime capture.
- Provide deterministic execution on systems without CUDA hardware by using a
  simulated GPU clock while still labelling queue domains correctly.
- Keep JSON output stable (`metadata`, `frames[]`, `summary`) so existing
  dashboards ingest the new scenario without schema drift.

Non-goals: shipping optimised CUDA kernels, modifying runtime submission
interfaces, or curating final benchmark datasets. Those remain follow-ups in
`AN-230.3+`.

## 2. Solution Overview

1. **Scenario Selection** — Add `--scenario=<cpu_baseline|gpu_async>` to the CLI
   and map it to an enum in the driver. The default remains `cpu_baseline` to
   keep scripts backwards compatible.
2. **Telemetry Model** — Introduce reusable structs under
   `engine::animation::benchmarking` for dispatch samples and aggregated queue/
   category totals. Provide helpers (`aggregate_category_totals`,
   `aggregate_queue_totals`) so both tests and the driver share the logic.
3. **GPU Capture Flow** — For each frame the GPU scenario will:
   - Advance the controller on the CPU (submission cost).
   - Dispatch a kernel through `compute::make_cuda_dispatcher()` that evaluates
     the pose. A simulated GPU clock wraps the kernel so timings are recorded in
     milliseconds while the dispatcher reports the `Gpu` timing domain.
   - Collect dispatcher telemetry and map it into dispatch/category/queue
     structures for JSON emission.
4. **JSON Writer Update** — Replace the hard-coded CPU-only emission logic with
   loops over the captured dispatch/aggregate structures so additional queues
   require no schema changes.
5. **Tests & Docs** — Add unit tests for the aggregation helpers and refresh the
   tool README / roadmap entries to describe the GPU scenario and new CLI option.

## 3. Data Flow

```
CLI (--scenario) ─► Scenario enum
                        │
                        ├── cpu_baseline: capture_cpu_frames()
                        │      (steady_clock timings, single CPU dispatch)
                        │
                        └── gpu_async: capture_gpu_frames()
                               │  advance_controller()  ──┐
                               │                          │
                               │  compute::Dispatcher ────┼─► ExecutionReport
                               │                          │       │
                               │                          └──► DispatchTelemetry
                               ▼
                  FrameTelemetry (dispatch list + aggregates)
                               │
                        FrameTimingSummary
                               │
                      JSON writer / console summary
```

## 4. Error Handling & Determinism

- Invalid `--scenario` values raise `std::invalid_argument`.
- When CUDA dispatchers are compiled out, we still produce GPU-labelled
  telemetry using the simulated clock so CI runs deterministically.
- All clocks derive from `steady_clock`; no random sampling is introduced.

## 5. Validation Plan

- Extend `engine_animation` unit tests to cover aggregation helpers.
- Run the benchmark driver in both scenarios and diff JSON payloads to confirm
  schema stability.
- Update roadmap/backlog entries marking `AN-230.2` as complete and referencing
  the diagnostics automation delivered in `AN-230.3` (`animation_sampling_report.py`).

## 6. Follow-up Work

- Replace simulated GPU kernels with actual CUDA implementations once the
  dispatcher backends expose device buffers.
- Automate scenario sweeps and integrate with the diagnostics dashboards via
  `scripts/diagnostics/animation_sampling_report.py`.
