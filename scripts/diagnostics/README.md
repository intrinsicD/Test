# Diagnostics Scripts

This directory hosts telemetry and profiling utilities that operate on the
engine's runtime libraries. The first tool, `runtime_frame_telemetry.py`, uses
the C ABI exposed by `engine_runtime` to capture dispatcher timings for the
animation → physics → geometry chain before render submission. The measurements
support the sprint 06 acceptance criterion for telemetry (`AI-003`, `RT-003`).

## `runtime_frame_telemetry.py`

1. **Build the engine** with a preset that produces shared libraries (for
   example `cmake --preset linux-clang-debug` followed by `cmake --build --preset
   linux-clang-debug --target engine_runtime`). Ensure the build output
   directory is discoverable through `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH`/`PATH`.
2. **Run the script** from the repository root:
   ```bash
   python scripts/diagnostics/runtime_frame_telemetry.py \
       --library-dir build/linux-clang-debug \
       --frames 8 --dt 0.016 --output telemetry/frame_timings.json
   ```
3. **Inspect the output**. The script prints aggregate timings by subsystem and
   stores detailed per-dispatch metrics in JSON when `--output` is specified. Use
   these artefacts to track regressions in the animation/physics ↔ rendering
   hand-off.

Use `--verbose` to emit per-frame tables on stdout when investigating specific
regressions. The JSON payload can be checked into performance dashboards or
post-processed by CI jobs for automated alerts.

## `streaming_report.py`

The streaming report surfaces queue depth and asynchronous asset loading
metrics gathered from the runtime thread pool and `AssetStreamingTelemetry`.

1. Build the runtime shared library using a preset that enables shared builds
   (for example `cmake --preset linux-gcc-debug`).
2. Run the script from the repository root:
   ```bash
   python scripts/diagnostics/streaming_report.py --library-dir out/build/linux-gcc-debug
   ```
3. Inspect the JSON payload printed to stdout or persisted via `--output`. The
   report includes worker counts, queue saturation, pending request totals, and
   cancellation/failure counters for asynchronous asset streaming.

Integrate the script into CI to monitor queue health once large streaming
workloads are exercised.
