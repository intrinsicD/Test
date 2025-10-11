# Animation Module Roadmap

## Context

The animation subsystem exposes deterministic clip sampling, a single linear controller, a smoke-test oscillator clip, and now a blend-tree authoring API capable of mixing multiple motion sources. Public headers provide data structures for joint poses, keyframes, and clips, and the runtime test suite validates interpolation for a single joint. JSON serialization is available alongside the shared I/O importer, but tooling for parameterised blending and deformation remains open.

## Objectives

1. **Asset and Clip Lifecycle**
   - ✅ Clip validation and JSON serialization now exist in `engine::animation` (`validate_clip`, `write_clip_json`, `read_clip_json`).
   - ✅ Provide import/export adapters within the `engine::io` module to integrate with asset pipelines and consider binary representations for runtime efficiency.

2. **Rig Evaluation and Blend Trees**
   - ✅ Design a node-based blend tree representation (clip/controller nodes and linear blend nodes are implemented; state machines remain future work).
   - ✅ Implement evaluators that operate over cached joint pose buffers with deterministic sampling order.
   - 🔜 Support parameter binding (floats, bools, events) for runtime control and debugging hooks.

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

Keep the aggregated backlog and the cross-subsystem view in [docs/global_roadmap.md](global_roadmap.md)
updated as these phases progress so animation stays aligned with dependent teams.

## Open Questions

- Which serialization format best balances readability and load-time performance?
- How should runtime controllers interact with the scheduler and job system (thread safety, caching)?
- What debugging interfaces (event timelines, pose inspection) are required for content creators?
