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

## TODO / Next Steps

- Track `RE-503`, `RE-510`, and the remaining `RT-003` follow-up (`RT-003.3`) in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — critical for `AI-003` and `RT-003`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RE-503` | Finalise resource metadata schema (`AI-003`). | Schema adopted by runtime + rendering with documentation. | ✅ Done |
| `RE-510` | Implement queue affinity validation. | Frame-graph rejects invalid transitions with regression coverage. | ✅ Done |
| `RE-520` | Update backend documentation. | Publish checklist + backend guides covering Vulkan parity. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for broader plan.
