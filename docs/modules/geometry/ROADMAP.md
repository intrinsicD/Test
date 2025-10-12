# Geometry Module Roadmap

## Near Term
- Finalise property registry ergonomics by adding typed accessors, iteration helpers, and validation coverage for mismatched attribute layouts.
- Optimise mesh normal recomputation and bounding volume updates for large meshes; benchmark against representative datasets.

## Mid Term
- Implement remeshing and parameterisation routines (isotropic, UV atlas) that feed animation skinning and physics collider generation.
- Extend kd-tree/octree utilities with dynamic updates to support streaming scenes and interactive editing.

## Long Term
- Deliver robust reconstruction pipelines (point cloud → watertight mesh) and integrate with IO detection to support scan ingestion workflows.
