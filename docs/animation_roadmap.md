# Animation Module Roadmap

## Context

The animation subsystem currently exposes deterministic clip sampling, a single linear controller, and a smoke-test oscillator clip. Public headers provide data structures for joint poses, keyframes, and clips, and the runtime test suite validates interpolation for a single joint. There is no persistent format, no rig authoring hooks, and no tooling for blending or deformation.

## Objectives

1. **Asset and Clip Lifecycle**
   - Formalise clip validation rules (unique joint names, monotonically increasing keyframes, consistent transforms).
   - Introduce serialization (JSON or binary chunk) for clips and rigs with versioned schemas.
   - Provide import/export adapters within the `engine::io` module to integrate with asset pipelines.

2. **Rig Evaluation and Blend Trees**
   - Design a node-based blend tree representation (state machine nodes, blend nodes, procedural nodes).
   - Implement evaluators that operate over cached joint pose buffers with deterministic sampling order.
   - Support parameter binding (floats, bools, events) for runtime control and debugging hooks.

3. **Deformation Integration**
   - Define rig binding data that links sampled poses to mesh skinning weights (interfacing with `engine::geometry`).
   - Surface GPU-friendly pose buffers for the rendering pipeline.
   - Add deformation utilities (dual quaternion blending, linear blend skinning) that consume controller output.

4. **Tooling and Diagnostics**
   - Expand unit tests to cover edge cases (empty tracks, non-looping playback, high-frequency sampling).
   - Create profiling scenarios (benchmarks, frame capture instrumentation) to validate performance envelopes.
   - Document workflows and authoring guidelines in module READMEs and design notes.

## Dependencies and Sequencing

1. Start with asset lifecycle improvements to unblock external data ingest.
2. Proceed with blend tree authoring once clip serialization is available for tools.
3. Integrate deformation paths after blend trees produce composite poses suitable for rendering.
4. Close with profiling and diagnostics to ensure stability before exposing tooling.

## Open Questions

- Which serialization format best balances readability and load-time performance?
- How should runtime controllers interact with the scheduler and job system (thread safety, caching)?
- What debugging interfaces (event timelines, pose inspection) are required for content creators?
