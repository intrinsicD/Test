# Rendering Module

## Current State
- Provides frame-graph compilation/execution, command encoder hooks, resource
  lifetime tracking, and a Vulkan-backed GPU scheduler prototype.
- Integrates with runtime submission paths and asset handles.
- Frame-graph compilation validates queue affinity and resource metadata to
  reject incompatible submissions early.
- The [`Frame-Graph Metadata Schema`](metadata_schema.md) documents resource and
  pass descriptors adopted by runtime and backend integrations (`RE-503`).
- The [`Vulkan Backend Checklist`](backend_checklist.md) captures prerequisites
  and validation steps for exercising the `RT-003` prototype end-to-end.

## Usage
- Build with `cmake --build --preset <preset> --target engine_rendering`.
- Include `<engine/rendering/frame_graph.hpp>` and related headers.
- Run `ctest --preset <preset> --tests-regex engine_rendering`.

## Handle Lifecycle Expectations (`AI-001`)
- Render-facing components never dereference asset storage directly; passes
  receive `engine::assets::ResourceHandle` instances through
  `RenderGeometry`, `MaterialSystem`, and scene components. The runtime and
  platform layers satisfy residency by routing these handles through
  `RenderResourceProvider` implementations that consult the asset caches.
- `RenderResourceProvider::require_*` contracts assume handles follow the
  generational semantics documented in the
  [resource management overview](../../design/resource_management.md). Provider
  implementations must validate handles (`handle.is_valid(pool)`) before copying
  data to the GPU; stale handles are rejected by the caches so rendering never
  consumes recycled slots.
- Systems keyed by handles (e.g., `MaterialSystem`) rely on identifier equality
  and expect caches to rebind handles after hot reloads. When runtime code
  registers materials or draw calls it should pass the most recent handle
  reference instead of caching raw pool indices.
- When an asset unloads, caches invoke `reset_binding()` to invalidate all live
  handles. Render passes must tolerate this by querying `RenderResourceProvider`
  every frame rather than storing raw pointers to GPU resources. Debug builds
  assert on invalid access, guiding developers back to the owning cache when
  validation trips.

## TODO / Next Steps

- Track `RE-530` backend validation tooling follow-up in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — critical for sustaining `RT-003` coverage.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RE-503` | Finalise resource metadata schema (`AI-003`). | Schema adopted by runtime + rendering with documentation. | ✅ Done |
| `RE-510` | Implement queue affinity validation. | Frame-graph rejects invalid transitions with regression coverage. | ✅ Done |
| `RE-520` | Update backend documentation. | Publish checklist + backend guides covering Vulkan parity. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for broader plan.
