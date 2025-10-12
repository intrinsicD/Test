# Engine Geometry Module

## Current State

- ✅ Core shape primitives (`Aabb`, `Sphere`, `Obb`, `Cylinder`, etc.) power bounds queries and collision scaffolding.
- ✅ Mesh utilities support surface/volumetric bounds, centroid extraction, and spatial indices (BVH, kd-tree prototypes).
- ✅ ASCII import/export paths round-trip meshes, graphs, and point clouds for toolchain validation.
- 🚧 API refinement in progress while naming and bounds types converge ahead of the v2.0 freeze.
- ⏳ Public documentation is staged under `include/engine/geometry/` and will be finalised once the stability goals below land.

Known limitations:
- `MeshBounds` and `Aabb` coexist and require consolidation before the ABI can be considered frozen.
- Header comments cover behaviour but lack the full complexity/precondition guidance expected for stable release notes.
- Shape benchmarks are sampled manually and do not yet run under the automated performance harness.

## API Stability

Target stable release: **v2.0** (see the [global roadmap](../../docs/global_roadmap.md)).

### Breaking changes planned before v2.0
- [ ] Replace `MeshBounds` with `Aabb` across mesh and import/export utilities (#123).
- [ ] Align naming conventions (PascalCase types, camelCase free functions) for all geometry headers (#124).
- [ ] Document `IOFlags` and serialization policies for public consumption (#125).

### Guarantees after v2.0
- Semantic versioning (`MAJOR.MINOR.PATCH`) with ABI compatibility within a major release.
- Deprecated features remain available for one major version before removal.
- Public headers ship with explicit layout/ownership annotations to guard DLL interoperability.

## Module Dependencies

```
geometry
├── depends on: math (vector/matrix/quaternion primitives)
├── used by: physics (collision detection)
├── used by: rendering (mesh data, bounds culling)
└── used by: animation (vertex deformation)
```

## Usage

The module builds as part of the aggregated engine configuration or can be compiled in isolation:

```bash
cmake -B build -S ../../ -DBUILD_GEOMETRY_ONLY=ON
cmake --build build --target engine_geometry
```

Link the produced target alongside `engine::project_options` when embedding it in external tooling.

## Public API Surface

- `<engine/geometry/api.hpp>` – Mesh definitions, property queries, import/export helpers.
- `<engine/geometry/shapes/*.hpp>` – Geometric primitives and associated algorithms.
- `<engine/geometry/random.hpp>` – Randomised shape generation for testing and sampling workflows.
- `<engine/geometry/detail/*>` – Internal headers; not part of the supported ABI.

## TODO / Next Steps

Keep these items synchronised with the workspace root backlog table and the global roadmap.

### Immediate (v2.0 readiness)
- [ ] Finalise mesh/shapes bounds consolidation (`MeshBounds` → `Aabb`) and update import/export tests (#123).
- [ ] Complete API naming audit and update documentation/comments to match conventions (#124).
- [ ] Publish `IOFlags` usage notes alongside serialization examples (#125).

### Short-term (post-freeze polish)
- [ ] Produce comprehensive documentation for every public header, including complexity and precondition notes.
- [ ] Add usage-driven samples demonstrating collision, rendering culling, and deformation integrations.
- [ ] Integrate geometry performance benchmarks into the automated harness.

### Long-term (roadmap alignment)
- [ ] Extend remeshing and graph refinement utilities aligned with physics interoperability goals.
- [ ] Deliver integration guides for physics and rendering modules that highlight data flow and ownership.
- [ ] Formalise ABI verification jobs in CI to track layout drift across toolchains.
