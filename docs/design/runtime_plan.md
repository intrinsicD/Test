# Runtime Module Roadmap

## Current Observations

- The module now exposes a `RuntimeHost` that owns subsystem dependencies, replacing the ad-hoc `runtime_state` singleton while still advancing animation, physics, geometry, and scene mirroring through the fixed kernel chain (`animation.evaluate → physics.accumulate → physics.integrate → geometry.deform → geometry.finalize`).
- Initialization constructs a toy physics world, generates a linear animation clip, and mirrors joint transforms into an EnTT-based scene registry, exposing the result through both C++ and C ABI front-ends.
- Shutdown simply resets transient buffers without releasing subsystem services or surfacing diagnostics, leaving lifecycle management, content streaming, and render scheduling unimplemented.

These constraints make the runtime suitable for smoke tests but insufficient for orchestrating production workloads.

## Near-Term Goals (1–2 Milestones)

1. **Formalise lifecycle management.** *(In progress — RuntimeHost landed)*
   - ✅ Replace the implicit singleton with an explicit `RuntimeHost` that owns subsystem handles (animation rig database, physics world pools, scene registry) and supports dependency injection.
   - ✅ Introduce idempotent `initialize()`/`shutdown()` semantics that propagate to dependent modules and expose status through the public API for validation.
   - Continue building out lifecycle diagnostics (logging, error propagation) and extend the gtests that now cover repeated initialize/shutdown cycles to also stress invalid usage sequences.

2. **Extend frame orchestration.**
   - Promote the dispatcher from a fixed linear chain to a graph driven by a frame graph description so the runtime can register optional subsystems (e.g., rendering, audio) without code changes.
   - ✅ Track per-kernel timing and surface it through `runtime_frame_state` for profiling hooks.
   - Validate ordering and data dependencies through additional gtests that assert topological execution and telemetry integrity.

3. **Scene synchronisation hardening.**
   - Support dynamic entity lifecycles (creation/destruction) when animation rigs change or content streams in, ensuring the mirror registry stays consistent.
   - Preserve author-specified hierarchy (parent indices) rather than mirroring a flat list, and expose stable identifiers so toolchains can track nodes across frames.
   - Add regression tests that construct rigs with branching hierarchies and verify world transforms.

## Mid-Term Goals (3–5 Milestones)

4. **Streaming asset integration.**
   - Interface with the assets and IO modules to asynchronously load rigs, meshes, and physics data, swapping them into the runtime without blocking the main loop.
   - Maintain double-buffered resource sets and add state transitions so partially loaded content cannot be consumed prematurely.
   - Cover the hot path with integration tests that simulate slow streams and assert that previously loaded content continues to animate correctly.

5. **Render scheduling hooks.**
   - Define a render submission interface (e.g., `RenderGraphBuilder`) that consumes the current scene snapshot, producing frame commands for the rendering module.
   - Propagate mesh bounds and camera configuration through the runtime state so visibility systems can execute before submission.
   - Prototype an end-to-end smoke test that exercises animation → physics → rendering to validate subsystem handshake.

6. **Diagnostics and tooling.**
   - Emit structured telemetry (JSON or binary capture) from the dispatcher and scene synchroniser, enabling external tools to visualise frame evolution.
   - Integrate hooks for the profiling tools module once available, providing markers around kernel execution and scene updates.
   - Expand documentation in `engine/runtime/README.md` with lifecycle diagrams and troubleshooting guides as features land.

## Long-Term Goals (5+ Milestones)

7. **High-frequency job system integration.**
   - Migrate the dispatcher onto the engine-wide task graph so runtime kernels can schedule work-stealing tasks across CPU cores and enqueue GPU dispatches through the compute module.
   - Implement dependency tracking between CPU and GPU stages, ensuring completion fences gate resource usage.

8. **Deterministic replay and state capture.**
   - Capture authoritative simulation inputs (controller parameters, physics forces, asset hashes) each frame so deterministic replays can be reproduced for debugging.
   - Persist snapshots to the tooling pipeline, enabling editor scrubbing and offline analysis.

9. **Hot-reloadable configuration.**
   - Expose configuration files or scripting bindings that allow designers to adjust runtime parameters (kernel order, timestep policy, streaming priorities) without recompilation.
   - Secure the interface with validation layers that reject incompatible changes at runtime.

## Documentation and Tracking

- Update the module README as milestones close, linking back to this roadmap to keep the aggregated backlog in the workspace root synchronised and consistent with the [central roadmap](../ROADMAP.md#subsystem-alignment).
- Mirror progress into the CI/backlog dashboards so dependent teams can align their deliverables.

