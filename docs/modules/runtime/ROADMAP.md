# Runtime Module Roadmap

_Last Updated: 2025-02-17 (Sprint 06 architecture/task audit)_

## Near Term (`RT-004`)
- [x] Ship lifecycle diagnostics via `RuntimeHost::diagnostics()` with subsystem timing and dispatcher stage telemetry mirrored through the C ABI and diagnostics scripts.
- [ ] Add validation around dependency resets (e.g., reloading meshes/controllers at runtime) to ensure state can be rebuilt without leaks.

## Mid Term
- Integrate asynchronous asset streaming so runtime can request geometry/animation data on demand while keeping simulation responsive (`AI-002`).
- ✅ Expose rendering submission hooks to feed frame graph passes once the GPU scheduler matures. `RuntimeHost::submit_render_graph` now drives the forward pipeline and Vulkan scheduler in tests; future work will focus on telemetry and multi-backend coverage (`RT-003`).

## Long Term
- Provide deterministic replay tooling (input capture, random seed control) and scripting hooks for automation/integration testing.
