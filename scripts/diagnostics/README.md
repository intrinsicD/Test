# Diagnostics Scripts

This directory hosts telemetry and profiling utilities that operate on the
engine's runtime libraries. The first tool, `runtime_frame_telemetry.py`, uses
the C ABI exposed by `engine_runtime` to capture dispatcher timings for the
animation → physics → geometry chain and now records lifecycle diagnostics from
`RuntimeHost::diagnostics()`. The measurements support the sprint 06 telemetry
acceptance criteria (`AI-003`, `RT-003`, `RT-004`).

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
   runtime lifecycle statistics (initialise/tick/shutdown durations plus
   per-stage samples), reports asynchronous streaming queue metrics harvested
   from `RuntimeDiagnostics`, enumerates structured metrics from the shared
   telemetry schema (lifecycle counters, streaming gauges, stage/subsystem
   samples, etc.), surfaces
   hierarchy validation issues forwarded by the diagnostics bridge (entity
   identifiers, relationship context, and messages), and stores detailed
   per-dispatch metrics in JSON when `--output` is specified. Use these
   artefacts to track regressions in the animation/physics ↔ rendering hand-off,
   monitor streaming health, triage hierarchy validation failures, and assess
   subsystem behaviour over time.

Use `--metric-prefix PREFIX` (repeatable) to restrict the printed metrics to
specific namespaces (for example `--metric-prefix runtime.lifecycle.`). Pass
`--metrics-all` to display the full metric set exposed by the runtime snapshot
instead of the default `runtime.streaming.*` subset.

Use `--verbose` to emit per-frame tables on stdout when investigating specific
regressions. The JSON payload can be checked into performance dashboards or
post-processed by CI jobs for automated alerts.

Set `--profile-trace trace.json` to export a Chrome trace containing per-dispatch
timings grouped by frame. The generated trace works with Perfetto, Chrome's
`about://tracing` viewer, and other tools that understand the trace event
schema, enabling deeper profiling workflows outside the default console
summary.

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
   report includes worker counts, queue saturation, pending request totals,
   cancellation/failure counters, and a `geometry_failures_by_error` mapping so
   operators can attribute failures to specific `GeometryIoErrorCode` values.

Integrate the script into CI to monitor queue health and failure attribution once
large streaming workloads are exercised.

## `telemetry_viewer.py`

The telemetry viewer provides the `CC-001` diagnostics shell MVP (`TL-101`). It
renders a textual dashboard from the JSON snapshots exported by
`runtime_frame_telemetry.py`, highlighting frame totals, streaming health,
dispatcher stages, subsystem timings, scene validation issues, and metric
samples filtered by prefix.

1. Export telemetry using `runtime_frame_telemetry.py` with the `--output`
   option.
2. Run the viewer from the repository root:
   ```bash
   python scripts/diagnostics/telemetry_viewer.py \
       --input telemetry/frame_timings.json \
       --metric-prefix runtime.streaming.
   ```
3. Inspect the console output for per-stage timings, streaming gauges, and
   filtered metrics. Adjust `--metric-prefix` (repeatable)—for example,
   `--metric-prefix runtime.geometry.spatial.` surfaces the octree telemetry—or
   `--max-issues` when triaging specific subsystems or scene hierarchy reports.
   When hot reload
   failures accumulate, the viewer adds a **Hot Reload Guidance** section that
   summarises failure counters, echoes the latest exported error (when
   available), and lists remediation steps (verify watcher permissions,
   re-export corrupt assets, increase queue capacity, etc.).
4. Provide `--verbose` to append metric description annotations when operators
   need additional context alongside sampled values. Combine with
   `--metric-prefix` filtering to keep verbose output focused.

## `collision_benchmark_report.py`

Use this reporter to surface collision throughput trends captured by
`physics_collision_benchmark`. It formats the benchmark JSON payload into a
human-readable summary and compares the latest run against a baseline to
highlight regressions or gains.

1. Run the benchmark (for example via CTest):
   ```bash
   ctest --preset linux-gcc-debug --tests-regex physics_collision_benchmark --output-on-failure
   ```
   The test emits `physics_collision_benchmark.json` inside the preset build
   directory.
2. Render the summary, optionally supplying a baseline for comparison:
   ```bash
   python scripts/diagnostics/collision_benchmark_report.py \
       --current out/build/linux-gcc-debug/physics_collision_benchmark.json \
       --baseline results/previous_run.json
   ```
3. Review the printed configuration, throughput, manifold/contact counts, and
   solver iteration deltas. Integrate the script into CI dashboards to track
   `PH-430` collision telemetry.

## `geometry_normals_benchmark_report.py`

Summarise the output of `geometry_normals_benchmark`, which records throughput
metrics for `geometry::recompute_vertex_normals` to advance `GE-205`/`TI-002`.

1. Execute the benchmark via CTest or by running the executable directly:
   ```bash
   ctest --preset linux-gcc-debug --tests-regex geometry_normals_benchmark --output-on-failure
   ```
   The run emits `geometry_normals_benchmark.json` inside the preset build
   directory.
2. Render the report, optionally providing a baseline for comparison:
   ```bash
   python scripts/diagnostics/geometry_normals_benchmark_report.py \
       --current out/build/linux-gcc-debug/geometry_normals_benchmark.json \
       --baseline results/previous_geometry_run.json
   ```
3. Inspect the configuration (grid resolution, iteration count, geometry size)
   and throughput metrics (iterations/vertices/triangles per second plus a
   checksum safeguarding determinism). Use the tool to track perf deltas across
   optimisation passes and surface regressions in CI dashboards once accelerated
   implementations land.
