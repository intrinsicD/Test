# Engine Geometry Public Headers

## Current State

- Exposes public headers that mirror the module API and guard ABI compatibility.
- Hosts subdirectories for Graph, Mesh, Octree, KdTree, Point Cloud, Properties, Shapes, Utils.

## Usage

- Include headers from `<engine/geometry/...>` when consuming the public API.
- Keep header changes paired with updates to the module tests and documentation.

## TODO / Next Steps

- Document the public headers once the API stabilises.
