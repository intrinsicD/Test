# Animation Benchmark Driver

`engine_animation_benchmark_driver` replays animation clips and records CPU
and GPU sampling telemetry for `AN-230`. The JSON payload mirrors the dispatcher
report from `engine_compute_runtime_sample`, making it compatible with
`scripts/diagnostics/compute_dispatch_report.py` and future dashboards.

## Build

```bash
cmake --build --preset <preset> --target engine_animation_benchmark_driver
# Example
cmake --build --preset linux-gcc-debug --target engine_animation_benchmark_driver
```

## Usage

```bash
./out/build/<preset>/engine/animation/engine_animation_benchmark_driver \
    --frames 2048 --dt 0.0166667 --clip assets/animation/walk.json \
    --rig-joints 60 --scenario gpu_async \
    --output telemetry/animation_gpu_async.json --pretty
```

### Options

- `--frames N` – number of frames to sample (default `1024`).
- `--dt SECONDS` – timestep between frames (default `1/60`).
- `--clip FILE` – path to an animation clip in JSON format. Falls back to
  `make_default_clip()` when omitted.
- `--rig-joints N` – joint count metadata for the harness report (defaults to
  the clip track count).
- `--output FILE` – JSON destination. When omitted, only the console summary is
  emitted.
- `--scenario NAME` – benchmark scenario (`cpu_baseline` or `gpu_async`).
- `--jitter-budget-ms VALUE` – frame jitter budget used to flag outliers in the
  summary (`0.5` ms by default).
- `--pretty` – prettify the JSON payload.
- `--help` – print the command reference.

The console summary lists frame count, aggregate timing statistics, and the
approximate frames-per-second throughput. The JSON payload exposes:

- `metadata` – task identifier, clip name/duration, rig and track counts.
- `summary.frame_totals_ms` – samples, min/max/mean/stddev, total time.
- `frames[]` – per-frame telemetry with dispatch/category/queue breakdowns that
  reuse the compute dispatcher schema for both CPU and GPU submissions.
- `frame_jitter_ms` + `frame_jitter_budget_ms` – mirrors dispatcher output to
  maintain CI parity.
- `metadata.cuda_available` – whether the CUDA dispatcher backend was compiled
  in when the capture was produced (the GPU scenario still runs deterministically
  when CUDA is unavailable).

Use the harness to compare CPU baselines against the simulated GPU dispatch
path. The GPU scenario uses the compute dispatcher to label work on the `gpu`
queue so diagnostics can ingest the data ahead of full CUDA kernel integration
in later `AN-230` tasks.

