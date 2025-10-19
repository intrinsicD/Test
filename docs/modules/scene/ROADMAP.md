# Scene Module Roadmap

## Near Term
- Formalise component schemas for lights, cameras, and visibility volumes; update systems/serialization to support the new types.
- Validation utilities now detect hierarchy cycles, missing parents, and transform inconsistencies; expand tooling surfaces that
  consume the reports and feed telemetry dashboards.

## Mid Term
- Extend serialization with versioning and migration paths to support backward/forward compatibility across saved scenes.
- Integrate scene queries (spatial partitioning, tagging) to accelerate runtime lookups.

## Long Term
- Author editor-facing tooling for scene graph inspection and manipulation, including undo/redo stacks and profiling instrumentation.
