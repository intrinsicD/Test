# Assets Module

## Current State
- Exposes generational `ResourceHandle<Tag>` wrappers for meshes, graphs, point
  clouds, textures, shaders, and materials. Handles bind lazily to
  `ResourcePool` instances, preventing stale references after unloads.
- Caches track descriptors, last-write timestamps, and hot-reload callbacks while
  delegating format-aware loading to `engine::io` utilities. Mesh and point cloud
  caches offer `load_async()` entry points backed by `AssetAsyncQueue`.
- Asset descriptors capture provenance, format hints, and binding metadata shared
  between caches and runtime consumers.
- Material assets currently store descriptor bindings (shader + texture handles);
  material authoring and serialization remain open.
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

## TODO / Next Steps

- Track `AS-302`, `AS-315`, `AS-320` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — aligns with `AI-002` and `CC-002`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AS-302` | Instrument async queue telemetry (`AI-002`). | Queue metrics emitted through runtime telemetry, documented in module README and streaming task file. | 🔄 In Progress |
| `AS-315` | Integrate hot reload callbacks (`CC-002`). | Filesystem watcher hooks update caches, failures logged via diagnostics shell. | 🟢 Todo |
| `AS-320` | Define material persistence strategy. | Draft design note covering serialization format and runtime reload semantics. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for sequencing and dependencies.
