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
    --frames 1024 --dt 0.016 --workload balanced --queues 3 \
    --output telemetry/compute_dispatch.json --pretty
```

Options:

- `--frames N` – number of ticks to simulate (default 1024)
- `--dt SECONDS` – timestep per tick (default 1/60)
- `--workload PROFILE` – workload intensity (`light`, `balanced`, `heavy`; default `balanced`)
- `--queues N` – logical compute queues to attribute telemetry to (default 1)
- `--queue-names LIST` – comma-separated queue names (e.g. `async-a,async-b`); overrides the default `queue-N` labels
- `--queue-map category=queue` – pin a category (e.g. `physics`) to a specific queue label
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

Combine the JSON output with `scripts/diagnostics/compute_dispatch_report.py`
to render tabular summaries or enforce jitter thresholds in CI.
