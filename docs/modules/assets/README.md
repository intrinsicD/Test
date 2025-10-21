# Assets Module

## Current State
- Exposes generational `ResourceHandle<Tag>` wrappers for meshes, graphs, point
  clouds, textures, shaders, and materials. Handles bind lazily to
  `ResourcePool` instances, preventing stale references after unloads.
- Caches track descriptors, last-write timestamps, and hot-reload callbacks while
  delegating format-aware loading to `engine::io` utilities. Mesh and point cloud
  caches offer `load_async()` entry points backed by `AssetAsyncQueue`, and all
  hot-reload capable caches subscribe to the platform filesystem watcher to
  trigger callbacks without manual polling loops.
- Asset descriptors capture provenance, format hints, and binding metadata shared
  between caches and runtime consumers.
- Material assets currently store descriptor bindings (shader + texture handles),
  and `AS-320` now codifies their authoring + serialization workflow in
  [`design/material_persistence_strategy.md`](../../design/material_persistence_strategy.md).
- Unit tests validate registration, reload behaviour, descriptor plumbing,
  generational semantics, and unload invalidation. Integration coverage flows
  through [`engine/tests/integration`](../../../engine/tests/integration/README.md).

## Usage
- Build via `cmake --build --preset <preset> --target engine_assets`; link against
  `engine_io`.
- Include `<engine/assets/handles.hpp>` plus relevant cache headers to request
  loads; include `<engine/assets/async.hpp>` for asynchronous scenarios.
- Run `ctest --preset <preset> --tests-regex engine_assets` to validate cache
  behaviour.
- Use `<engine/assets/async.hpp>` to create `AssetLoadRequest` descriptors and
  `AssetLoadFuture` channels when scheduling asynchronous work.
- Inspect async streaming telemetry with
  ``python scripts/diagnostics/streaming_report.py --library-dir <build>`` to
  review queue occupancy, completion totals, failures, cancellations, and
  rejection counters emitted by `AssetStreamingTelemetry`.

## Handle Lifecycle Guidance (`AI-001`)
- Asset handles are lightweight identifiers backed by
  `engine::assets::ResourceHandle<Tag>`; creation only stages the identifier and
  defers binding until a cache loads the descriptor. See the
  [resource management overview](../../design/resource_management.md) for the
  generational handle model adopted across modules.
- Cache entry points such as `MeshCache::load` acquire a
  `core::memory::ResourcePool` slot, bind the handle via `handle.bind(raw)`, and
  immediately merge pending hot-reload callbacks so all outstanding references
  share the new generation.
- Callers must treat handles as opaque tokens and always verify residency with
  `handle.is_valid(pool)` (mirroring the internal `contains` checks) before
  dereferencing cache state. When a resource is unloaded the cache invokes
  `handle.reset_binding()`; stale handles therefore fail validation instead of
  aliasing recycled storage.
- Hot-reload and asynchronous queues reuse the same lifecycle: callbacks staged
  against identifiers migrate to the live generational slot when a load
  completes, and async completions deliver rebound handles inside
  `AssetLoadResult` structures. Runtime integrations should rely on these
  hand-offs instead of copying raw pool indices.
- Debug builds assert on invalid access, while release builds surface structured
  errors (`AssetLoadError`) so higher layers can attribute failure without
  crashing. Documentation and telemetry updates must preserve these semantics
  when expanding the async queue (`AI-002`).
- Caches register validators with the handle validation registry, enabling
  runtime and tooling code to call `engine::assets::validate_handle` for debug
  assertions and telemetry counters (`runtime.handles.*`) when stale handles are
  detected.

## TODO / Next Steps

- Track `AS-302`, `AS-315`, `AS-320` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — aligns with `AI-002` and `CC-002`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AS-302` | Instrument async queue telemetry (`AI-002`). | Queue metrics emitted through runtime telemetry, documented in module README and streaming task file. | ✅ Done |
| `AS-315` | Integrate hot reload callbacks (`CC-002`). | Filesystem watcher hooks update caches, failures logged via diagnostics shell. | ✅ Done |
| `AS-320` | Define material persistence strategy. | Draft design note covering serialization format and runtime reload semantics. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for sequencing and dependencies.
