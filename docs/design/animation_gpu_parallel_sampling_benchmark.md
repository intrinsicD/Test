# AN-230: GPU Parallel Sampling Benchmark Plan

_Last Updated: 2025-03-24_

## Purpose

Roadmap item `AN-230` kicks off the animation module's GPU and multi-core
sampling initiative. The goal is to prototype a benchmarking methodology that
quantifies controller throughput when work is dispatched through
`compute::KernelDispatcher` onto asynchronous compute queues. This document
captures the experimental design, shared assumptions with neighbouring tasks,
and the deliverables required before implementation work begins.

## Scope & Goals

- Establish benchmark cases that stress clip sampling, blending, and
  pose post-processing under realistic rig topologies (10–120 joints).
- Compare CPU baseline controllers against GPU compute kernels with
  varying queue depths and synchronization primitives.
- Produce reproducible metrics: sampled frames per second, queue
  occupancy, memory bandwidth, and latency jitter.
- Validate telemetry integration by publishing results through the
  existing diagnostics pipeline (`CC-001`).

### Non-Goals

- Shipping production-ready GPU sampling kernels (tracked under future
  AN-23x tasks).
- Optimising runtime/renderer interop beyond benchmarking harness needs.
- Revisiting deformation accuracy (covered by `ADR-0006`).

## Shared Assumptions

Coordination with `AN-240` is limited to compute queue capabilities. Both
tracks assume:

- `compute::KernelDispatcher` exposes at least two asynchronous compute
  queues with timeline semaphore synchronization against the graphics
  queue.
- Queue submissions support fence-based completion callbacks so the
  animation runtime can recycle staging buffers deterministically.
- GPU memory budgets for animation workloads cap at 256 MiB per frame for
  skinning matrices and temporary pose data.

These assumptions will be validated during prototype spikes and captured in
telemetry output for traceability.

## Benchmark Matrix

| Scenario | Rig Complexity | Workload | Data Sets | Metrics |
| --- | --- | --- | --- | --- |
| Baseline CPU | 15, 60 joint rigs | Single clip sampling | Humanoid walk, creature idle | FPS, CPU time, cache misses |
| GPU Async (single queue) | 15, 60, 120 joint rigs | Clip sampling + skinning buffer writeback | Same as baseline + stress clip | FPS, GPU time, queue occupancy |
| GPU Async (multi-queue) | 60, 120 joint rigs | Clip blending (2–4 layers) | Layered combat, locomotion | FPS, GPU time, semaphore wait ratios |
| Hybrid CPU/GPU | 60 joint rigs | CPU sampling + GPU pose post | Mixed mocap set | FPS, end-to-end latency, bandwidth |

Each scenario will be executed at three animation lengths (10 s, 60 s,
looped) and across cold cache/warm cache cycles to capture stability.

## Methodology

1. **Harness Construction** — Extend `engine/animation/tools/benchmark_driver`
   (new utility) to replay animation clips, dispatch compute kernels, and
   record telemetry snapshots.
2. **Dataset Preparation** — Curate representative clips in
   `assets/animation/benchmarks/` with metadata describing rig size,
   compression, and joint topology.
3. **Instrumentation** — Integrate `engine::telemetry::ScopeTimer` around
   CPU code paths and GPU timestamp queries around compute queue submissions.
   Emit structured metrics tagged by scenario, rig complexity, and queue
   configuration.
4. **Execution Protocol** — Run each scenario for 1,000 frames after warmup,
   repeating three times. Capture raw telemetry logs and aggregated reports.
5. **Analysis** — Use `python/tools/analyze_animation_bench.py` to compute
   averages, percentiles, and regressions relative to CPU baselines.

## Deliverables

- Benchmark harness sources and build target integrated into CMake.
- Dataset manifest documenting rig statistics and licensing.
- Telemetry dashboards (Grafana folder `Animation/GPU Sampling Bench`).
- Summary report embedded into `docs/modules/animation/README.md` once data
  stabilises.

## Risks & Mitigations

- **Hardware Variability:** Mitigate via pinned reference hardware (RTX 4080,
  Radeon 7900 XT) and driver version checks baked into harness startup.
- **Queue Starvation:** If asynchronous compute interferes with graphics
  queue, fall back to serialized submissions while capturing contention data
  for follow-up investigations.
- **Data Volume:** Raw telemetry may exceed storage budgets; compress logs and
  retain only aggregated metrics after validation.

## Timeline & Checkpoints

- Week 1: Harness skeleton, dataset ingestion, CPU baseline runs.
- Week 2: GPU kernel integration with single queue; telemetry validation.
- Week 3: Multi-queue experiments, hybrid mode, draft report.
- Week 4: Finalize report, publish dashboards, capture lessons learned for
  subsequent implementation tasks (`AN-232`, `AN-233`).

## Open Questions

- Precise memory layout for pose staging buffers shared between CPU and GPU.
- Whether we need cross-vendor shader permutations for compute kernels.
- Impact of driver-level preemption on queue latency (requires lab access).
