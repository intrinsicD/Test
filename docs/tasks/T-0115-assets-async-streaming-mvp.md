# T-0115: Async Streaming MVP Thread Pool & Telemetry

## Goal
Advance `AI-002` by delivering the remaining infrastructure for asynchronous asset streaming:
- Dedicated IO thread pool integrated with runtime configuration.
- Asset cache state machine for pending → loading → ready transitions.
- Priority queue scheduling and cancellation semantics.
- Telemetry instrumentation and reporting script updates.

## Background
- Design reference: [`docs/design/async_streaming.md`](../design/async_streaming.md).
- Current status: `AssetLoadRequest`/`AssetLoadFuture` primitives exist; scheduler, thread pool, and telemetry hooks are missing.
- Dependencies: Resource lifetime management (`AI-001`) guarantees handle safety; error handling (`DC-004`) standardises
  `Result<T, Error>` usage.

## Inputs
- Code: `engine/assets/async.hpp`, `engine/assets/src/`, `engine/core/threading/`, `engine/runtime/runtime_host.cpp`,
  `scripts/diagnostics/`.
- Build configs: ensure new thread pool compiles under all presets.
- Docs: `docs/modules/assets/README.md`, `docs/modules/runtime/README.md`, `docs/ROADMAP.md`.

## Constraints
- Thread pool must support deterministic sizing via configuration (no reliance on hardware concurrency when tests demand fixed
  counts).
- Use non-blocking synchronization primitives where possible; avoid busy-wait loops.
- Cancellation must be safe even when decode work is mid-flight—caches must gate binding on cancellation checks.
- Telemetry output should integrate with existing JSON schema produced by `runtime_frame_telemetry.py` (extend schema if needed).

## Deliverables
1. **IO Thread Pool (`engine::core::threading::IoThreadPool`)**
   - Configurable worker count, queue depth, and priority buckets.
   - Lifecycle managed by `RuntimeHost` (configure on initialize, teardown on shutdown).
2. **Asset Cache State Machine**
   - Extend caches with pending binding records, cancellation hooks, and validation for dependency completion.
   - Ensure handles only bind on success; publish structured `AssetLoadError` on failure.
3. **Scheduler Implementation**
   - Priority queue supporting reprioritisation and cancellation; instrumentation for queue depth.
   - Worker loop executing cache-provided decode callbacks with try/finally semantics for cleanup.
4. **Telemetry & Diagnostics**
   - Emit events for queue, dispatch, completion, cancellation.
   - Update or author `scripts/diagnostics/streaming_report.py` to summarise active/past requests.
   - Document telemetry schema and add sample output to this task file.
5. **Testing**
   - Unit tests for thread pool configuration and cancellation.
   - Integration test hooking runtime streaming request path with deterministic fixtures.
   - Regression coverage for failure cases (IO error, validation error, cancellation).
6. **Documentation**
   - Update assets and runtime READMEs with configuration instructions and telemetry usage.
   - Reference completion in central roadmap.

## Work Breakdown
1. Thread pool scaffolding and configuration integration.
2. Asset cache extensions for pending bindings and cancellation.
3. Scheduler logic with priority queue and worker loops.
4. Telemetry instrumentation plus diagnostics script implementation.
5. Testing matrix (unit + integration) and documentation updates.

## Acceptance Criteria
- [x] Runtime-configurable IO thread pool with deterministic sizing and teardown.
- [x] Asset caches support async load lifecycle with cancellation and structured errors.
- [x] Telemetry script reports queue depth, throughput, and failure breakdowns.
- [x] Tests cover success, failure, and cancellation paths and run under CI presets.
- [x] Documentation cross-links roadmap `AI-002` and telemetry tooling.

## Metrics & Benchmarks
- Record throughput (requests/sec) for mesh + animation loads across debug/release builds.
- Monitor average queue wait time and worker utilization over 500 requests.
- `python scripts/diagnostics/streaming_report.py --library-dir out/build/linux-gcc-debug` on the
  configured debug build emits `worker_count=2`, `queue_capacity=64`, and zero in-flight requests
  immediately after startup, establishing the baseline for regression tracking.

## Open Questions
- Do we expose per-asset-type worker pools or a shared pool with weighted priorities?
- Should telemetry integrate with existing runtime telemetry stream or remain a standalone report initially?
- How do we gate asynchronous loads in deterministic replay scenarios?
