# Assets Module Roadmap

## Near Term
- Extend the cache system to track non-geometry assets (textures, shaders, materials) with timestamp monitoring and callbacks, mirroring the facilities implemented for meshes.
- Surface import diagnostics by attaching loader provenance, error codes, and detected formats to `MeshAsset` and friends so tooling can report actionable failures.

## Mid Term
- Integrate dependency graphs between assets (for example textures referenced by materials) and invalidate caches transitively when upstream content changes.
- Implement background refresh polling that amortises filesystem scans and exposes profiling hooks for asset IO costs.

## Long Term
- Persist cooked artefacts (e.g., compressed meshes, shader bytecode) alongside source descriptors to accelerate runtime startup and support deterministic streaming.
