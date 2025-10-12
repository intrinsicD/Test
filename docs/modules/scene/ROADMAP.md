# Scene Module Roadmap

## Near Term
- Formalise component schemas for lights, cameras, and visibility volumes; update systems/serialization to support the new types.
- Add validation utilities that detect hierarchy cycles, missing parents, and transform inconsistencies.

## Mid Term
- Extend serialization with versioning and migration paths to support backward/forward compatibility across saved scenes.
- Integrate scene queries (spatial partitioning, tagging) to accelerate runtime lookups.

## Long Term
- Author editor-facing tooling for scene graph inspection and manipulation, including undo/redo stacks and profiling instrumentation.
