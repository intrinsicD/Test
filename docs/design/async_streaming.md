# Async Asset Streaming Architecture (AI-002)

**Status:** MVP Implemented – thread pool, cache scheduling, and telemetry tooling landed in
`engine/core`, `engine/assets`, `engine/runtime`, and `scripts/diagnostics/streaming_report.py`.

**Last Updated:** 2025-02-18

## Overview

This document defines the asynchronous asset streaming architecture that fulfils
roadmap item `AI-002`. The goal is to let runtime and tooling code request asset
loads without blocking the main thread while preserving the deterministic
lifetime guarantees established by `AI-001`. The design introduces request and
future abstractions, a dedicated IO work scheduler, and telemetry that binds
asset caches, the IO module, and runtime orchestration together.

## Goals

- Allow any subsystem (runtime, tools, editor) to enqueue asset loads and poll
  completion without stalling the main thread.
- Preserve deterministic lifetimes by binding streamed assets to generational
  handles only after successful import and validation.
- Support prioritisation, cancellation, and batching so hosts can balance
  loading latency with CPU/GPU workload.
- Surface progress, errors, and provenance through telemetry aligned with the
  existing `engine::Result<T, Error>` error-handling policy.
- Keep integration points narrow: IO handles decoding, assets bind/cache
  resources, runtime consumes handles, and the scheduler orchestrates work.

## Non-Goals

- Implement GPU upload streaming; the initial scope ends once an asset is in the
  CPU-side cache and ready for GPU residency.
- Replace existing synchronous `load()` helpers. They remain valid for tests and
  tooling scenarios that prefer blocking semantics.
- Define editor UI/UX. The document only exposes APIs and telemetry hooks that
  tooling can query.

## Background

The assets module exposes caches backed by `ResourcePool` and
`ResourceHandle<Tag>` wrappers (`AI-001`). Caches currently load assets
synchronously via IO helpers. Runtime subsystems (animation, rendering) expect
handles to be valid when returned; cache insertion happens during the blocking
load. As workloads scale, the synchronous path starves the runtime thread and
prevents overlapping IO. `AI-002` introduces asynchronous primitives that layer
on top of existing caches without changing their generational guarantees.

## Requirements

1. **Deterministic handles.** Handles created before a load must remain
   unbound until the asynchronous request succeeds. Generation counters must only
   advance inside the cache on success.
2. **Thread safety.** The scheduler and caches need explicit synchronisation.
   Cache mutation occurs on worker threads; readers on the main thread must see
   the committed asset atomically.
3. **Configurable concurrency.** Hosts set maximum worker counts and queue
   depth. Default sizing targets one IO thread per available hardware context,
   with overridable presets for deterministic tests.
4. **Priorities and cancellation.** Requestors can reprioritise or cancel work.
   Cancellation must occur before IO commits results to caches.
5. **Diagnostics.** Every request reports timestamps (queued, dispatched,
   completed), source identifiers, errors, and retry counts. Telemetry surfaces
   through `scripts/diagnostics/` tooling and runtime APIs.
6. **Extensibility.** New asset types (materials, audio, etc.) reuse the same
   scheduler; caches provide type-specific decode functions.

## Architecture Overview

### Entry Points

Two new primitives anchor the API:

- `AssetLoadRequest` – immutable description of the work. Fields include:
  - `AssetType type` – mesh, point cloud, shader, etc.
  - `std::string identifier` – canonical cache key (e.g., path or GUID).
  - `AssetImportParams import_params` – format hints, dependency overrides.
  - `AssetLoadPriority priority` – enum bucket (`High`, `Normal`, `Low`).
  - `bool allow_blocking_fallback` – permit synchronous resolution if the
    scheduler is disabled.
  - `std::chrono::steady_clock::duration deadline` – optional soft deadline for
    reprioritisation heuristics.

- `AssetLoadFuture` – handle returned to callers. Provides:
  - `bool is_ready() const` / `void wait()` – completion checks.
  - `Result<ResourceHandle<Tag>, AssetLoadError> get()` – obtains the bound
    handle or structured error.
  - `AssetLoadState state() const` – exposes `Pending`, `Loading`,
    `Ready`, `Failed`, `Cancelled` enumerants.
  - `ProgressInfo progress() const` – IO byte counts, dependency completion.
  - `void cancel()` – request cancellation while the job is pending or loading.

Futures are reference-counted so multiple consumers can observe progress. They
contain a shared state object guarded by a mutex/condition variable.

### Scheduler and Thread Pool

A new `engine::core::threading::IoThreadPool` exposes:

- `static IoThreadPool& instance()` – singleton configured during runtime
  startup. Presets now live directly on `RuntimeHostDependencies::streaming_config`.
- `void configure(const IoThreadPoolConfig&)` – sets worker count, queue depth,
  and per-thread affinity. The runtime initialises the pool during
  `RuntimeHost::initialize()` and tears it down on shutdown.
- `bool enqueue(IoTaskPriority, std::function<void()>)` – inserts work into a
  priority queue (highest priority first, FIFO within the same bucket). When the
  queue is saturated the call returns `false`, allowing caches to fall back to
  synchronous execution when explicitly permitted.

Workers execute a standard pipeline:

1. Resolve the cache responsible for the request (mesh, shader, etc.).
2. Acquire or create the pending `ResourceHandle<Tag>` without binding.
3. Invoke the cache’s asynchronous import hook, which delegates to IO decoders.
4. On success, bind the handle inside the cache and notify the future.
5. On failure, populate the future with `AssetLoadError` and trigger telemetry.
6. Emit completion events to the diagnostics subsystem.

Thread safety is ensured by:

- Reusing cache-level mutexes to serialise inserts/updates.
- Publishing results via atomics before notifying waiters.
- Using lock-free queues for worker wakeups while guarding priority changes with
  a lightweight spinlock or `std::mutex` (implementation detail left to the
  execution phase).

### Cache Integration

Each cache owns an `AssetAsyncQueue<Handle>`:

- `AssetAsyncQueue::schedule()` packages cache-specific decode lambdas and
  returns an `AssetLoadFuture`. Mesh and point cloud caches now share this
  implementation, reusing the existing synchronous `load()` helpers inside the
  queued job.
- The queue tracks identifier → state transitions (`Pending → Loading → Ready`)
  and deduplicates concurrent requests by handing out additional futures backed
  by the same shared state.
- Cancellation signals propagate through `AssetLoadPromise` callbacks; when the
  queue notices a cancellation before the decode begins the state moves directly
  to `Cancelled` without performing IO.

Caches continue to own descriptor parsing and import logic, so the scheduler
remains format agnostic. Material/shader caches will adopt the same queue in a
future iteration once their async loaders land.

### Telemetry & Diagnostics

All state transitions generate events consumed by:

- Runtime telemetry: `RuntimeHost::streaming_metrics()` exposes per-request and
  aggregated timings.
- Scripts: `scripts/diagnostics/streaming_report.py` queries the runtime C API
  and reports worker counts, queue occupancy, and per-state totals for
  asynchronous requests.
- Logging: worker threads emit structured logs via `spdlog` with request IDs.

Telemetry payload fields include request ID, identifier, type, priority,
queue/dispatch/completion timestamps, bytes transferred, and error codes.

### Cancellation & Prioritisation

- `AssetLoadFuture::cancel()` marks the shared state as cancelled and removes
  the job from the queue if it has not started. If already loading, caches check
  `AssetLoadState::cancelled` between pipeline stages and abort before binding.
- Reprioritisation occurs by enqueuing an updated request descriptor. Internally
  the scheduler stores heap nodes per request so priority changes adjust heap
  ordering.

### Interaction with Runtime and Other Subsystems

- Runtime exposes `RuntimeHost::request_asset_load(const AssetLoadRequest&)`
  which forwards to the owning cache and registers the future with runtime
  diagnostics.
- Animation/physics subsystems can subscribe to future completion signals to
  delay evaluation until assets are ready. Integration-specific hooks will be
  designed per subsystem but all rely on the same future interface.
- IO module continues to provide synchronous decoders. Asynchronous paths wrap
  them in background tasks; streaming-specific decoders (e.g., chunked mesh
  loaders) can be introduced later without altering the API.

## Error Handling

`AssetLoadError` extends `engine::ErrorCode` and captures:

- `AssetLoadError::Category` – `IoFailure`, `DecodeError`, `ValidationError`,
  `Cancelled`, `Timeout`.
- `std::string message` – human-readable diagnostic.
- `std::filesystem::path source` – original file path, if applicable.
- `std::vector<std::string> dependency_failures` – identifiers of failed
  sub-requests.

Futures return `Result<ResourceHandle<Tag>, AssetLoadError>` from `get()`. Call
sites can branch on error category to retry, fall back to placeholder assets, or
propagate the failure upstream.

## API Sketch

```cpp
struct AssetLoadRequest {
    AssetType type;
    std::string identifier;
    AssetImportParams import_params;
    AssetLoadPriority priority = AssetLoadPriority::Normal;
    std::optional<std::chrono::steady_clock::duration> deadline;
    bool allow_blocking_fallback = false;
};

class AssetLoadFuture {
public:
    bool is_ready() const;
    void wait() const;
    AssetLoadState state() const;
    ProgressInfo progress() const;
    Result<ResourceHandleBase, AssetLoadError> get();
    void cancel();
    void reprioritise(AssetLoadPriority new_priority);
};

AssetLoadFuture RuntimeHost::request_asset_load(const AssetLoadRequest& request);
AssetLoadFuture MeshCache::schedule_async(const AssetLoadRequest& request);
```

The concrete handle type returned by `get()` depends on the cache. Template
helpers (e.g., `AssetLoadFutureT<MeshHandle>`) provide type-safe wrappers while
sharing the same underlying shared state.

## Testing Strategy

- **Unit tests** in `engine/assets/tests/` cover request lifecycle transitions,
  cancellation, and priority ordering using a deterministic single-threaded mock
  scheduler.
- **Integration tests** simulate runtime usage: enqueue loads, tick the runtime,
  and assert that caches bind handles before dependent systems consume them.
- **Failure injection tests** stub IO decoders to return specific error codes and
  validate telemetry/logging output.
- **Performance benchmarks** (later in AI-002) measure throughput and contention
  to tune default worker counts.

## Phased Implementation Plan

1. **Design foundation (this document).** Establish API contracts and data
   flow. ✅
2. **Thread pool & scheduler core.** Introduce `IoThreadPool`, priority queue,
   and shared future state.
3. **Cache integration.** Add `schedule_async` to each cache and wrap existing
   synchronous imports in background tasks.
4. **Runtime exposure.** Surface `RuntimeHost::request_asset_load` and update
   diagnostics to report streaming metrics.
5. **Telemetry tooling.** Implement scripts/diagnostics coverage per sprint
   acceptance criteria.
6. **Performance tuning.** Add benchmarks, configuration presets, and profiling
   guidance.

## Open Questions

- Should deadlines trigger automatic cancellation or priority boosts? Initial
  implementation favours priority boosts to avoid surprising cancellations.
- How should dependent requests (e.g., materials referencing shaders) propagate
  progress? Proposal: parent futures aggregate the slowest child and expose
  dependency counts.
- Do we require persistence for partially streamed assets? Deferred until asset
  cook/pipeline work in later milestones.

## Risks & Mitigations

- **Contention on cache mutexes.** Mitigate by minimising the critical section to
  binding + handle updates; decoding occurs outside locks.
- **Thread pool starvation.** Allow configuration of worker count and queue depth
  at runtime; expose diagnostics to detect backlog buildup.
- **Error propagation complexity.** The structured `AssetLoadError` carries
  provenance and dependency failures so hosts can render actionable UI/logs.

## References

- `AI-001` Resource management design (`docs/design/resource_management.md`).
- Error handling policy (`docs/design/error_handling_migration.md`).
- Runtime module overview (`docs/modules/runtime/README.md`).
