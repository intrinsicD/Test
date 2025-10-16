# Assets Module Roadmap

## Near Term
- Implement asynchronous streaming primitives per [`docs/design/async_streaming.md`](../../design/async_streaming.md) so caches can service `AssetLoadRequest` futures without blocking the main thread (aligns with `AI-002`).
- Add material asset persistence and hot-reload by introducing descriptor serialization plus file-backed change detection that mirrors the existing mesh/point-cloud/shader caches.
- Consolidate duplicated cache lifecycle logic (load, poll, reload) into shared helpers to reduce maintenance overhead across asset types.
- Surface import diagnostics by attaching loader provenance, error codes, and detected formats to `MeshAsset` and friends so tooling can report actionable failures.

## Mid Term
- Integrate dependency graphs between assets (for example textures referenced by materials) and invalidate caches transitively when upstream content changes.
- Implement background refresh polling that amortises filesystem scans and exposes profiling hooks for asset IO costs.

## Long Term
- Persist cooked artefacts (e.g., compressed meshes, shader bytecode) alongside source descriptors to accelerate runtime startup and support deterministic streaming.
