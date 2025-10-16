# Resource Management Overview

## Motivation

Asset caches previously keyed resources by opaque string identifiers. While the
approach simplified initial experiments, it provided no guard rails against
stale references—strings remained "valid" even after the underlying resource was
unloaded and reinserted. The new resource management layer introduces
generational handles backed by a `ResourcePool` so stale references are rejected
deterministically and caches can recycle storage without exposing indices.

## Generational Handles and ResourcePool

`engine::core::memory::ResourcePool<T, Tag>` owns a dense set of resources of
type `T`. Each slot is identified by a `GenerationalHandle<Tag>` consisting of an
index and a generation counter.

Key properties:

- `acquire(args...)` constructs a resource in place and returns both the handle
  and a reference. Slots are recycled via a free list; releasing a handle bumps
  the generation counter so previous handles become invalid.
- `is_valid(handle)` verifies that a handle references a live resource by
  checking the generation and occupancy flag.
- `get(handle)` returns references with debug assertions guarding misuse.
- `for_each` iterates over live slots without exposing the internal storage.
- Debug builds assert on invalid access, while release builds throw
  `std::out_of_range` to prevent undefined behaviour.

This pool is intentionally minimal—modules can layer domain-specific semantics
over the deterministic lifetime semantics it provides.

## Asset Handles

`engine/assets/handles.hpp` wraps the generational handles with
`ResourceHandle<Tag>`. Asset handles retain the user-facing identifier
(`std::string` or path) and bind lazily once a cache loads the asset:

- Handles can be created before a resource is loaded, enabling hot-reload
  subscriptions ahead of the first `load` call.
- All handle copies share state through `std::shared_ptr`, so caches updating
  the binding automatically update descriptors, tests, and runtime references.
- `is_bound()` reports whether a handle currently maps to a live slot, while
  `is_valid(pool)` verifies both the binding and the underlying pool state.
- `reset_binding()` is invoked by caches when unloading assets to invalidate all
  outstanding references.

## Cache Lifecycle Changes

All asset caches now:

1. Maintain a `ResourcePool` alongside maps from identifiers to raw handles.
2. Support registering hot-reload callbacks before the first load by staging
   callbacks in a pending map keyed by identifier.
3. Rebind handles on every load, ensuring descriptors and external references
   share the updated generational state.
4. Move callbacks back to the pending queue when unloading so future loads reuse
   subscriptions.
5. Guard `get`/`contains` against stale handles using `ResourcePool::is_valid`.

The lifecycle guarantees now align with the roadmap requirement from `AI-001`:
asset handles are type-safe, bound to deterministic lifetimes, and trivially
auditable during debugging.

## Debug Validation

The pool injects assertions in `get` and `release`, ensuring invalid handles trip
fast in debug builds. Release builds throw exceptions so call sites can surface
errors without crashing. Tests in `engine/core/tests/resource_pool_tests.cpp`
exercise slot reuse and iteration, while `engine/assets/tests/test_assets.cpp`
verifies handle binding semantics, unload invalidation, and pre-load callback
registration.

## Migration Guidance

- Construct asset handles from identifiers or paths as before; caches bind them
  to concrete slots on first load.
- Replace direct lookups against `std::unordered_map<Handle, Asset>` with
  `ResourcePool` queries and `ResourceHandle::raw_handle()` when internal
  storage needs to index by handle.
- Prefer storing callbacks against raw handles in caches while staging
  pre-binding callbacks via the identifier map.
- When introducing new caches, start from the updated patterns in
  `mesh_asset.cpp` or `shader_asset.cpp` to ensure pending callbacks and
  identifier bookkeeping stay consistent.

