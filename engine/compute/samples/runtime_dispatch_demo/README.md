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
    --frames 512 --dt 0.016 --output telemetry/compute_dispatch.json --pretty
```

Options:

- `--frames N` – number of ticks to simulate (default 256)
- `--dt SECONDS` – timestep per tick (default 1/60)
- `--output FILE` – path to write the JSON telemetry payload
- `--pretty` – emit indented JSON when writing to `FILE`
- `--help` – display the command reference

Without `--output`, the sample prints a console summary with the most
expensive kernels and per-category totals. When `--output` is supplied the JSON
payload captures every frame, per-dispatch durations (milliseconds), category
roll-ups, and runtime stage timings harvested from
`RuntimeHost::diagnostics()`.

Combine the JSON output with `scripts/diagnostics/compute_dispatch_report.py`
to render tabular summaries or enforce jitter thresholds in CI.
