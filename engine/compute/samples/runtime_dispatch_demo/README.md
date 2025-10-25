# Runtime Dispatch Demo

The `engine_compute_runtime_sample` executable advances `RuntimeHost` for a
configurable number of ticks and records dispatcher telemetry for the
animation → physics → geometry pipeline. Use it to validate queue orchestration
and to seed telemetry captures for follow-up analysis (`CO-170`).

## Build

```bash
cmake --build --preset <preset> --target engine_compute_runtime_sample
# Example
cmake --build --preset linux-gcc-debug --target engine_compute_runtime_sample
```

## Usage

```bash
./out/build/<preset>/engine/compute/engine_compute_runtime_sample \
    --frames 1024 --dt 0.016 --workload balanced --dispatcher-backend cpu --queues 3 \
    --output telemetry/compute_dispatch.json --pretty
```

Options:

- `--frames N` – number of ticks to simulate (default 1024)
- `--dt SECONDS` – timestep per tick (default 1/60)
- `--workload PROFILE` – workload intensity (`light`, `balanced`, `heavy`; default `balanced`)
- `--dispatcher-backend BACKEND` – dispatcher backend (`cpu`, `cuda`; default `cpu`)
- `--queues N` – logical compute queues to attribute telemetry to (default 1)
- `--queue-names LIST` – comma-separated queue names (e.g. `async-a,async-b`); overrides the default `queue-N` labels
- `--queue-map category=queue` – pin a category (e.g. `physics`) to a specific queue label
- `--jitter-budget-ms VALUE` – maximum allowed frame dispatch jitter σ in milliseconds before warnings are emitted (default 0.5)
- `--baseline` – capture a single-queue baseline and report the achieved speed-up versus the optimised run (target 1.5×)
- `--output FILE` – path to write the JSON telemetry payload
- `--pretty` – emit indented JSON when writing to `FILE`
- `--help` – display the command reference

Without `--output`, the sample prints a console summary with the most
expensive kernels, per-category totals, queue assignments, and queue aggregates.
When `--output` is supplied the JSON payload captures every frame, per-dispatch durations
(milliseconds), queue and category roll-ups, and runtime stage timings
harvested from `RuntimeHost::diagnostics()`.

Cross-queue dependencies are recorded explicitly: the summary lists every
logical fence where work crosses queue boundaries and the JSON payload exposes
`queue_dependencies` (aggregated counts and consuming kernels) alongside the
`queue_transitions` edge list. This instrumentation makes it possible to audit
timeline semaphore sequencing and validate multi-queue orchestration in
telemetry dashboards.

Queue attribution falls back to a deterministic FNV-1a hash so categories that
are not explicitly mapped still resolve to stable queue indices across runs and
platforms, ensuring telemetry comparisons remain reproducible. Select
`--dispatcher-backend cuda` once the CUDA dispatcher is available to compare GPU
and CPU execution characteristics; the JSON payload and CLI summary record the
backend for both the multi-queue run and the single-queue baseline so reports
remain self-describing.

Combine the JSON output with `scripts/diagnostics/compute_dispatch_report.py`
to render tabular summaries or enforce jitter thresholds in CI.

When `--baseline` is supplied the sample performs an additional run using a
single logical queue, records the baseline frame timing statistics, and compares
them against the multi-queue capture. The console summary and JSON payload list
the baseline average/min/max frame times, jitter, and the observed speed-up with
the `1.5×` target highlighted so regressions surface immediately in CI reports.
The runtime enforces the jitter budget by default (0.5 ms σ at 60 FPS): the
console summary and JSON metadata flag both the optimised run and the baseline
when dispatch-to-completion jitter exceeds the configured budget so CI captures
latency regressions automatically.
