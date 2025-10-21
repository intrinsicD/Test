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
  is discoverable through the platform library search path or pass it via
  `--library-dir`.
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
- Supply `--profile-trace trace.json` to export the per-dispatch timings as a
  Chrome trace, enabling analysis with Perfetto/`about://tracing` alongside the
  console summary.

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

### Troubleshooting
- **Runtime library cannot be located.** The diagnostics scripts raise
  `RuntimeError: Unable to load runtime library 'engine_runtime'` when the
  shared object is missing from the search path. Rebuild the runtime target,
  supply `--library-dir <build/output>` explicitly, and export
  `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH`/`PATH` so the loader resolves the
  library when subprocesses are spawned. On Linux, run
  ``ldd out/build/linux-gcc-debug/libengine_runtime.so`` to confirm dependency
  resolution; on Windows, ensure the build directory that holds
  `engine_runtime.dll` is present in `PATH`.
- **Metrics appear truncated.** Without prefixes the script defaults to the
  `runtime.streaming.` namespace. Provide additional `--metric-prefix` values or
  enable `--metrics-all` to print the entire telemetry snapshot when debugging
  lifecycle, stage, or hierarchy signals.
- **Variance checks fail unexpectedly.** Increase the sample window with
  `--frames`, relax the threshold passed to `--variance-check`, or trim warm-up
  noise by combining `--variance-check geometry.deform:5` with
  `--variance-trim 0.1`. These options operate entirely on the captured timing
  samples, so they do not mask genuine regressions.
- **Headless runs crash during window creation.** Force the mock backend via
  `--window-backend mock` (default) or export
  `ENGINE_PLATFORM_WINDOW_BACKEND=mock` before launching telemetry capture.
  This bypasses native surface requirements while preserving deterministic
  dispatcher behaviour.
- **Retain console output for audits.** Use `--output` to persist JSON snapshots,
  pass `--verbose` to capture per-frame tables in log archives, and add
  `--profile-trace trace.json` when a Chrome trace is required for
  post-mortems. These artefacts integrate with the diagnostics viewer and
  regression dashboards referenced by TL-101/TL-110.

## TODO / Next Steps

- Track `TL-101`, `TL-110`, `TL-115` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — required for `CC-001` viewer work.
- Expose metric description annotations in the diagnostics viewer verbose mode to address the outstanding review feedback for TL-101.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `TL-101` | Stand up diagnostics shell MVP (`CC-001`). | CLI/UI viewer renders telemetry, smoke tests documented. | ✅ Done |
| `TL-110` | Document tooling invocation. | Update README with commands, environment requirements, and troubleshooting. | ✅ Done |
| `TL-115` | Profiling capture export. | Implement export path with regression coverage. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for sequencing.
