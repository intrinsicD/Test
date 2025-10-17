# Runtime Module Roadmap

## Near Term
- Improve lifecycle diagnostics by logging initialization sequencing, dependency wiring, and per-tick timing; expose hooks for embedding applications to capture telemetry.
- Add validation around dependency resets (e.g., reloading meshes/controllers at runtime) to ensure state can be rebuilt without leaks.

## Mid Term
- Integrate asynchronous asset streaming so runtime can request geometry/animation data on demand while keeping simulation responsive.
- ✅ Expose rendering submission hooks to feed frame graph passes once the GPU scheduler matures. `RuntimeHost::submit_render_graph` now drives the forward pipeline and Vulkan scheduler in tests; future work will focus on telemetry and multi-backend coverage.

## Long Term
- Provide deterministic replay tooling (input capture, random seed control) and scripting hooks for automation/integration testing.
