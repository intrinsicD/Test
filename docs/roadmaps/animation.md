# Animation Module Roadmap

## Status Overview
- ✅ Phase 1: Foundation (Completed M1–M2)
- 🔄 Phase 2: Integration (In Progress M3)
- ☐ Phase 3: Advanced Features (Planned M4–M6)

## Completed (M1–M2)
- ✅ Clip validation and JSON serialization
- ✅ Linear blend tree nodes
- ✅ Parameter binding (float, bool, event)
- ✅ Deterministic sampling
- ✅ Controller advancement and evaluation

## Phase 2: Integration (M3 – Due 2025-11-15)

### Additive Blend Nodes
- **Owner:** @animation-team
- **Issue:** #234
- **Tasks:**
  - [ ] Design additive pose composition API
  - [ ] Implement `BlendTreeAdditiveNode`
  - [ ] Add unit tests for additive blending
  - [ ] Document usage in API reference

### Deformation Binding
- **Owner:** @bob
- **Issue:** #236
- **Tasks:**
  - [ ] Define `RigBinding` data structure
  - [ ] Link rig poses to mesh vertices
  - [ ] Implement linear blend skinning
  - [ ] Add integration test with geometry module

## Phase 3: Advanced Features (M4–M6)

[Continue with clear phase breakdown]

## Dependencies
- **Geometry module:** Required for deformation (skinning needs mesh access)
- **Rendering module:** Required for GPU pose buffers
- **IO module:** Already integrated for clip serialization

## Design Rationale
- Serialization remains JSON-first for readability; binary formats are deferred until asset tooling demands faster ingest.
- Runtime controllers interact with the scheduler via immutable parameter binding to keep evaluation thread-safe while enabling future job-graph integration.
- Debugging interfaces (event timelines, pose inspection) are planned alongside editor tooling so instrumentation lands with authoring workflows.
