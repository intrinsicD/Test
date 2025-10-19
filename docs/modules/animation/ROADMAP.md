# Animation Module Roadmap

_Last Updated: 2025-02-17 (Sprint 06 architecture/task audit)_

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
  - [x] Design additive pose composition API
  - [x] Implement `BlendTreeAdditiveNode`
  - [x] Add unit tests for additive blending
  - [x] Document usage in API reference

### Deformation Binding (`RT-001` alignment)
- **Owner:** @bob
- **Issue:** #236
- **Tasks:**
  - [x] Define `RigBinding` data structure (tracks `RT-001` progress)
  - [x] Link rig poses to mesh vertices
  - [x] Implement linear blend skinning
  - [x] Add integration test with geometry module

### Clip Validation Hardening
- **Owner:** @animation-team
- **Issue:** #238
- **Tasks:**
  - [x] Emit structured error codes from `validate_clip`
  - [ ] Cover empty-track and unordered-key edge cases in tests
  - [ ] Extend controller regression tests for validation failures

## Phase 3: Advanced Features (M4–M6)

### GPU and Parallel Sampling
- Batch `sample_clip` requests via `compute::KernelDispatcher` and benchmark controller evaluation throughput across representative scenes.

### State Machine Authoring APIs
- Introduce transition-driven orchestration for `AnimationBlendTree` nodes, including condition evaluation, event propagation, and preview tooling hooks.

### Advanced Deformation Pipelines
- Extend pose data structures for dual quaternion skinning, curve-driven rigs, and compatibility layers that integrate geometry/physics representations.

## Dependencies
- **Geometry module:** Required for deformation (skinning needs mesh access)
- **Rendering module:** Required for GPU pose buffers
- **IO module:** Already integrated for clip serialization

## Design Rationale
- Serialization remains JSON-first for readability; binary formats are deferred until asset tooling demands faster ingest.
- Runtime controllers interact with the scheduler via immutable parameter binding to keep evaluation thread-safe while enabling future job-graph integration.
- Debugging interfaces (event timelines, pose inspection) are planned alongside editor tooling so instrumentation lands with authoring workflows.
