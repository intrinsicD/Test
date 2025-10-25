# AN-230 Benchmark Harness Design

**Status:** Implemented (CPU baseline + GPU telemetry scenarios merged)
**Owner:** Animation/Compute Tech Lead  
**Related Tasks:** `AN-230.1` (GPU sampling benchmarks – harness & CPU baselines)

## 1. Problem & Goals

`AN-230.1` requires a repeatable way to benchmark animation clip sampling so
that subsequent GPU work has trustworthy CPU baselines. The harness must:

- Replay representative clips with deterministic timing.
- Record per-frame sampling costs compatible with the `CO-170` telemetry
  pipeline (frame totals, category breakdowns, jitter metrics).
- Produce console and JSON summaries that future automation (dashboards,
  CI regressions) can consume.

Non-goals: GPU kernels, dispatcher integration, or dataset curation (tracked by
follow-up subtasks).

## 2. Proposed Solution

Create `engine_animation_benchmark_driver`, an executable living in
`engine/animation/tools/benchmark_driver/`. Key responsibilities:

1. **CLI ingestion** – flags for clip path, frame count, timestep, output file,
   pretty-printing, and rig metadata.
2. **Sampling loop** – uses `AnimationController` to advance time and evaluates
   poses each frame. Default clip comes from `make_default_clip()` when no asset
   is provided.
3. **Telemetry capture** – records per-frame CPU durations (steady clock) and
   aggregates summary statistics via a reusable helper in
   `animation::benchmarking::compute_frame_timing_summary`.
4. **Reporting** – prints summary totals and optionally writes JSON shaped like
   the dispatcher report (`frames[]`, `dispatches[]`, `frame_totals_ms`, jitter
   metadata). Each frame emits a single dispatch entry (`animation.sample_clip`)
   so downstream analysis reuses the `CO-170` tooling untouched.

## 3. Data Flow & Structures

```
CommandLineOptions ─┐
                     │    AnimationClip (JSON | default)
        Clip Loader ─┼─► AnimationController ─► Sampling Loop ─► FrameTelemetry
                     │                                 │
Rig metadata (CLI) ──┘                                 ▼
                                   benchmarking::compute_frame_timing_summary
                                                    │
                                                    ▼
                                   Console summary & JSON payload
```

- `FrameTelemetry`: `{ index, simulation_time, timestep, sample_ms }`.
- `FrameTimingSummary`: `{ samples, total_ms, mean_ms, min_ms, max_ms,
  stddev_ms }`.
- JSON structure mirrors `engine_compute_runtime_sample` so that
  `compute_dispatch_report.py` can parse the file without modifications.

## 4. Error Handling & Determinism

- Validate CLI inputs (`frames > 0`, `timestep > 0`, filesystem paths).
- Wrap clip loading in `try/catch` to surface parse errors with actionable
  messages.
- Seed nothing – the sampling path is deterministic. Jitter only reflects CPU
  scheduling.

## 5. Build, Test & Documentation

- **Build:** extend `engine/animation/CMakeLists.txt` with a `tools` subdirectory
  and expose the new executable (`engine_animation_benchmark_driver`).
- **Library helper:** new header/source pair under `animation/benchmarking/`
  provides summary statistics for reuse and unit testing.
- **Tests:** add `test_benchmark_statistics.cpp` covering zero/positive sample
  paths using GoogleTest.
- **Docs:** update roadmap (`AN-230.1`), animation README/backlog, and add a
  tool README describing CLI usage and integration with `CO-170` telemetry.

## 6. Follow-up (Out of Scope)

- GPU kernel dispatch (`AN-230.2`), dispatcher wiring, and queue attribution.
- Dataset manifest & asset ingestion (clip libraries, rig metadata).
- Automation scripts turning harness output into dashboards (`AN-230.3`) via
  `scripts/diagnostics/animation_sampling_report.py`.

