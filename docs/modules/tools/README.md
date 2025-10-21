# Tools Module

## Current State
- Staging area for editor, profiling, and pipeline automation features with
  roadmap-driven scaffolding.
- Houses initial diagnostics scripting hooks, including the telemetry viewer
  CLI that renders runtime snapshots exported by
  `scripts/diagnostics/runtime_frame_telemetry.py`.
- Shares the cross-module telemetry instrumentation guidance captured in
  [`docs/design/telemetry_instrumentation_guide.md`](../../design/telemetry_instrumentation_guide.md).

## Usage
- Build with `cmake --build --preset <preset> --target engine_tools`.
- Run Python tooling under `python/tools/` and scripts in `scripts/` as
  documented by specific utilities.

### Capture runtime telemetry snapshots
- Configure a preset that produces shared runtime libraries (for example,
  `cmake --preset linux-gcc-debug` followed by `cmake --build --preset
  linux-gcc-debug --target engine_runtime`). Ensure the build output directory
  is discoverable through the platform library search path.
- Collect telemetry via:
  ```bash
  python scripts/diagnostics/runtime_frame_telemetry.py \
      --library-dir out/build/linux-gcc-debug \
      --frames 16 --dt 0.016 --output telemetry/frame_timings.json
  ```
- By default the script prints metrics whose fully-qualified names start with
  `runtime.streaming.`. Repeat `--metric-prefix` to inspect other namespaces
  (for example, `--metric-prefix runtime.lifecycle.`). Pass `--metrics-all` to
  dump every metric from the runtime snapshot when investigating broader
  anomalies.

### Monitor streaming diagnostics
- Summarise the asynchronous asset queue via:
  ```bash
  python scripts/diagnostics/streaming_report.py --library-dir out/build/linux-gcc-debug
  ```
- The report surfaces worker counts, queue capacity, pending requests, and
  cancellation/failure counters emitted by `AssetStreamingTelemetry`.

### Inspect telemetry archives interactively
- Feed exported telemetry snapshots to the diagnostics shell:
  ```bash
  python scripts/diagnostics/telemetry_viewer.py \
      --input telemetry/frame_timings.json \
      --metric-prefix runtime.streaming.
  ```
- Adjust `--metric-prefix` (repeatable) to focus on specific subsystems and use
  `--max-issues` to expand hierarchy validation summaries when triaging scene
  diagnostics.

## TODO / Next Steps

- Track `TL-101`, `TL-110`, `TL-115` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — required for `CC-001` viewer work.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `TL-101` | Stand up diagnostics shell MVP (`CC-001`). | CLI/UI viewer renders telemetry, smoke tests documented. | ✅ Done |
| `TL-110` | Document tooling invocation. | Update README with commands, environment requirements, and troubleshooting. | 🟡 In Progress |
| `TL-115` | Profiling capture export. | Implement export path with regression coverage. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for sequencing.
