# Animation Benchmark Driver

`engine_animation_benchmark_driver` replays animation clips and records CPU
sampling telemetry for `AN-230`. The JSON payload mirrors the dispatcher report
from `engine_compute_runtime_sample`, making it compatible with
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
    --rig-joints 60 --output telemetry/animation_cpu_baseline.json --pretty
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
- `--pretty` – prettify the JSON payload.
- `--help` – print the command reference.

The console summary lists frame count, aggregate timing statistics, and the
approximate frames-per-second throughput. The JSON payload exposes:

- `metadata` – task identifier, clip name/duration, rig and track counts.
- `summary.frame_totals_ms` – samples, min/max/mean/stddev, total time.
- `frames[]` – per-frame telemetry, each with a single `animation.sample_clip`
  dispatch entry so `compute_dispatch_report.py` can ingest the file without
  modification.
- `frame_jitter_ms` + `frame_jitter_budget_ms` – mirrors dispatcher output to
  maintain CI parity.

Use the harness to capture CPU baselines before integrating GPU kernels in
`AN-230.2`.

