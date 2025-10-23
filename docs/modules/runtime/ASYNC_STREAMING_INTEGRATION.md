# Runtime Async Streaming Integration Guide

**Status:** Published – fulfils `AI-002.3` by documenting runtime expectations for
asynchronous asset streaming.

_Last Updated: 2025-02-20_

## Scope

This guide explains how the runtime consumes the asynchronous asset streaming
infrastructure introduced in `AI-002`. It is aimed at subsystem owners wiring
asset requests through the runtime orchestration layer, tooling engineers who
need to monitor queue health, and QA engineers building deterministic test
harnesses. The guide covers configuration, request lifecycles, telemetry, and
validation workflows.

Before proceeding, familiarise yourself with the architectural design in
[`docs/design/ASYNC_STREAMING.md`](../../design/ASYNC_STREAMING.md) and the asset
module overview in [`docs/modules/assets/README.md`](../assets/README.md).

## Prerequisites

- `engine/assets` caches expose `load_async()` entry points for meshes and point
  clouds. Additional asset types reuse the same `AssetAsyncQueue` scaffolding.
- The IO thread pool (`engine::core::threading::IoThreadPool`) is compiled into
  the runtime build; no extra configuration flags are required.
- Runtime builds that ship a shared library (`engine_runtime`) make telemetry
  data available to scripts through the public C API.
- Resource lifetime guarantees from `AI-001` remain in effect: handles bind only
  after a successful import and validation pass.

## Configuring the IO Thread Pool

`RuntimeHost` configures the IO thread pool during `initialize()` using the
values stored in `RuntimeHostDependencies::streaming_config`:

```cpp
engine::runtime::RuntimeHostDependencies deps{};
deps.streaming_config.worker_count = 2;          // defaults to 2 workers
deps.streaming_config.queue_capacity = 64;       // pending requests before rejection
deps.streaming_config.enable = true;             // disable to force synchronous loads

engine::runtime::RuntimeHost runtime{std::move(deps)};
runtime.initialize();
```

Key behaviours:

- **Deterministic sizing** – Tests should explicitly set `worker_count` and
  `queue_capacity` to avoid relying on hardware concurrency.
- **Graceful opt-out** – Set `enable = false` when asynchronous work must be
  disabled (for example, deterministic replay). Caches honour
  `allow_blocking_fallback` on individual requests to fall back to synchronous
  loads when the queue is saturated or disabled.
- **Teardown** – `RuntimeHost::shutdown()` stops the thread pool and clears
  pending work. Callers must not rely on outstanding futures completing after
  shutdown.

## Scheduling Asset Requests

Subsystems can either call cache APIs directly or delegate to the runtime host.
Provide cache instances through `RuntimeHostDependencies::asset_streaming` and
use the convenience wrappers when the runtime should own the scheduling:

```cpp
engine::assets::MeshCache mesh_cache;
engine::runtime::RuntimeHostDependencies deps{};
deps.asset_streaming.mesh_cache = &mesh_cache;

engine::runtime::RuntimeHost host{std::move(deps)};
host.initialize();

auto request = engine::assets::AssetLoadRequest::from_path(
    engine::assets::AssetType::mesh,
    "assets/meshes/rigged_character.glb",
    /*params*/ {},
    engine::assets::AssetLoadPriority::High,
    /*deadline*/ std::nullopt,
    /*allow_blocking_fallback*/ true);

engine::assets::AssetLoadFuture<engine::assets::MeshHandle> future = host.request_mesh_asset(request);
```

`AssetAsyncQueue` deduplicates concurrent requests by identifier and tracks the
state machine (`Pending → Loading → Ready/Failed/Cancelled`). Futures surface
completion via `is_ready()`, `wait()`, and `get()`; callers must handle
`Result<ResourceHandle<Tag>, AssetLoadError>` outcomes and propagate structured
errors upstream. When the runtime host is not initialised or required caches are
absent, `request_mesh_asset()` and `request_point_cloud_asset()` return futures
containing `AssetLoadErrorCategory::ValidationError` results immediately and
increment rejection telemetry instead of throwing exceptions. Cancellation
requests transition futures to `Cancelled` as long as the worker has not
committed the result. When subsystems require bespoke queues they can still
invoke `MeshCache::load_async()` and
`PointCloudCache::load_async()` directly, supplying the IO thread pool singleton.

## Runtime Diagnostics and Telemetry

Runtime metrics combine IO thread pool statistics with
`AssetStreamingTelemetry` counters. Access them through either the C++ or C
APIs:

```cpp
const auto metrics = engine::runtime::streaming_metrics();
fmt::print("workers={}, pending={}, completed={}\n",
           metrics.worker_count,
           metrics.streaming_pending,
           metrics.streaming_total_completed);
```

```c
struct engine_runtime_streaming_metrics metrics;
engine_runtime_streaming_metrics(&metrics);
printf("pending=%" PRIu64 "\n", metrics.streaming_pending);
```

Include `<inttypes.h>` before using the `PRIu64` macro in C integrations.

Fields include worker counts, queue capacity, pending/active tasks, total jobs
enqueued/executed, per-state streaming totals (completed, failed,
cancelled, rejected), and geometry failure attribution arrays. The C++ struct
exposes `streaming_geometry_failures` while the C ABI surfaces a
`geometry_failures_by_error` dictionary keyed by `GeometryIoErrorCode` strings
so tooling can identify the dominant failure modes. Metrics refresh automatically when the runtime
initialises, ticks, or shuts down; hosts can call `engine::runtime::streaming_metrics()`
for an up-to-date snapshot at any time.

### Diagnostics Scripts

`scripts/diagnostics/streaming_report.py` queries the C API and prints metrics in
JSON form. Typical usage:

```bash
python scripts/diagnostics/streaming_report.py --library-dir out/build/linux-gcc-debug
```

Integrate the script into CI to monitor queue saturation, failure attribution, and queue pressure. For
deep dives, pair it with `scripts/diagnostics/runtime_frame_telemetry.py`, which
includes streaming metrics (including `geometry_failures_by_error` mappings) alongside frame timings and hierarchy diagnostics.

## Error Handling Expectations

- Cache jobs return `AssetLoadResult<Handle>` values. Successful loads bind the
  handle generation and publish completion telemetry.
- Failures categorise errors as `IoFailure`, `DecodeError`, or
  `ValidationError`, including contextual messages (path, reason).
- Cancellation is explicit: futures transition to `Cancelled` and emit telemetry
  without touching cache state.
- Queue rejection increments the `streaming_total_rejected` counter and returns a
  failure result. Callers should honour `allow_blocking_fallback` when retries
  are unacceptable.

## Testing & Validation

- Unit tests targeting `engine/assets/tests/test_async.cpp` validate queue
  semantics, telemetry counters, and cancellation behaviour. Extend these tests
  when adding new asset types.
- Runtime integration suites (`engine/runtime/tests/test_module.cpp`) assert that
  streaming metrics mirror diagnostics state. Update them if new counters are
  introduced.
- Run `python scripts/diagnostics/streaming_report.py --library-dir <build>` in
  CI to detect regressions in worker configuration or queue utilisation.
- Execute `python scripts/validate_docs.py` after editing documentation to
  maintain cross-reference integrity.

## Troubleshooting Checklist

1. **Pending requests never complete** – Verify the thread pool is enabled and
   worker count > 0. Inspect runtime logs for cache validation errors.
2. **Queue rejection spikes** – Increase `queue_capacity` or lower request
   volume. Confirm callers are not issuing duplicate identifiers unnecessarily.
3. **Telemetry stuck at zero** – Ensure `RuntimeHost::initialize()` executed.
   Streaming metrics reset to zero after shutdown.
4. **Deterministic tests flake** – Fix worker counts and disable asynchronous
   loads unless the test covers streaming explicitly. Use
   `AssetStreamingTelemetry::reset_for_testing()` to clear counters between
   assertions.

Cross-link anomalies and follow-up tasks back to the central roadmap (`AI-002`)
so that future iterations track outstanding streaming work.
