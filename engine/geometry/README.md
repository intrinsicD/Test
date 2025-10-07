# Geometry

_Path: `engine/geometry`_

_Last updated: 2025-02-14_


## Overview

The geometry module implements the surface and volumetric kernels required by the engine. Key facilities include:

- A half-edge mesh core with property maps (`include/engine/geometry/mesh/halfedge_mesh.hpp`) supporting arbitrary
  attribute attachment.
- Generic property registries that expose strongly-typed handles for component data.
- Basic analytic shapes (sphere, cylinder, ray) and point-cloud containers for procedural content and intersection
  testing.
- Surface sampling, decimation, UV unwrapping, and topology helpers under the `src/` subtree.

## Usage

Link against `engine_geometry` and include headers from `include/engine/geometry/`:

```cmake
target_link_libraries(my_app PRIVATE engine_geometry)
```

```cpp
#include <engine/geometry/mesh/halfedge_mesh.hpp>

engine::geometry::HalfedgeMesh mesh;
auto position = mesh.request_vertex_property<engine::math::vec3>("v:position");
```

The accompanying GoogleTest suites under `tests/` exercise mesh traversal, property registry lifetime semantics, and
discrete differential geometry utilities.
