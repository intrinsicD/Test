# Geometry

_Path: `engine/geometry`_

_Last updated: 2025-03-15_


## Overview

Geometry provides both heavy-weight mesh infrastructure and the lightweight runtime mesh used by the sample
simulation. In addition to the half-edge mesh and property registries, the module now exposes
[`SurfaceMesh`](include/engine/geometry/api.hpp) – a contiguous vertex/index container with runtime deformation
helpers.

Key facilities include:

- Half-edge mesh kernels with property maps (`include/engine/geometry/mesh/halfedge_mesh.hpp`).
- Shape primitives and sampling utilities under `include/engine/geometry/shapes/`.
- SurfaceMesh helpers for procedural generation (`make_unit_quad`), translation (`apply_uniform_translation`), and
  normal/bounds updates used directly by the runtime tests.

## Usage

Link against `engine_geometry` and include headers from `include/engine/geometry/`:

```cmake
target_link_libraries(my_app PRIVATE engine_geometry)
```

```cpp
#include <engine/geometry/api.hpp>

auto mesh = engine::geometry::make_unit_quad();
engine::geometry::apply_uniform_translation(mesh, {0.0F, 1.0F, 0.0F});
engine::geometry::recompute_vertex_normals(mesh);
```

`update_bounds` now normalizes degenerate meshes with no vertices to the origin, guaranteeing that
`MeshBounds::min` and `MeshBounds::max` never expose `±std::numeric_limits<float>` sentinels to
callers that perform downstream culling or collision tests.

## TODO

- Add non-uniform deformation helpers so animation weights can drive vertex blends.
- Extend `SurfaceMesh` to cache rest-state tangents for normal-mapped shading.
- Port more of the legacy topology utilities onto the new `SurfaceMesh` façade to avoid duplication.
