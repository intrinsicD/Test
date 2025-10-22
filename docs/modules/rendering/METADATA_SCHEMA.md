# Frame-Graph Metadata Schema

_Last Updated: 2025-02-19_

The rendering module exposes a frame-graph that the runtime and tooling use to
describe rendering workloads in a backend-agnostic way. This document
summarises the metadata schema so authors and reviewers can align on the
contract that satisfies roadmap item `RE-503` (`AI-003`).

## Resources

Frame-graph resources are declared through
`engine::rendering::FrameGraphResourceDescriptor` and are exposed to passes as
immutable `FrameGraphResourceInfo` snapshots.

| Field | Type | Description |
| --- | --- | --- |
| `name` | `std::string` / `std::string_view` | Stable identifier for the resource. Used by passes, serialization, and backend translations. |
| `lifetime` | `ResourceLifetime` (`External`, `Transient`) | Indicates whether the resource is imported by the host or allocated for the graph’s duration. |
| `format` | `ResourceFormat` | Encodes pixel / texel format for textures and attachments (`Rgba8Unorm`, `Rgba16f`, `Depth24Stencil8`, …). |
| `dimension` | `ResourceDimension` | Declares the shape of the resource (`Buffer`, `Texture1D`, `Texture2D`, `Texture3D`, `CubeMap`). |
| `usage` | `ResourceUsage` bitmask | Describes how passes will access the resource (transfer, shader read/write, color/depth attachments, presentation). |
| `initial_state` | `ResourceState` | Resource state expected before the first use (e.g., `ColorAttachment`, `ShaderRead`). |
| `final_state` | `ResourceState` | State the graph promises to leave the resource in after the last writer. |
| `width`/`height`/`depth` | `std::uint32_t` | Dimensions for texture resources. Buffers leave these fields at `1`. |
| `array_layers` | `std::uint32_t` | Number of array slices for layered textures (defaults to `1`). |
| `mip_levels` | `std::uint32_t` | Mip-chain length (defaults to `1`). |
| `sample_count` | `ResourceSampleCount` | Multisample count (1×, 2×, 4×, …). |
| `size_bytes` | `std::uint64_t` | Raw byte size for buffers. Unused for textures. |

The same structure backs both descriptors and info objects so helpers can be
implemented without duplicating conversion logic. Helper utilities exist to
print human-readable representations for logging and serialization.

## Pass Metadata

Each pass derives from `engine::rendering::RenderPass` and records metadata
that feeds scheduling and diagnostics:

| Field | Type | Description |
| --- | --- | --- |
| `name()` | `std::string_view` | Stable debug label emitted during execution, serialization, and telemetry. |
| `queue()` | `QueueType` | Preferred submission queue (`Graphics`, `Compute`, `Transfer`). Propagated into GPU submit descriptors. |
| `phase()` | `PassPhase` | High-level stage (`Setup`, `Geometry`, `Lighting`, `PostProcess`, `Compute`, `Transfer`, `Presentation`). Aids frame capture tooling and scheduling visualisations. |
| `validation_severity()` | `ValidationSeverity` | Hint for diagnostics consumers when the pass emits validation findings (`Info`, `Warning`, `Error`). |

During compilation `FrameGraphPassBuilder` collects the read/write sets declared
by each pass. Execution contexts expose accessors so passes can query resource
metadata (`FrameGraphPassExecutionContext::describe`) alongside queue and phase
information when recording commands.

## Serialization

Compiled frame-graphs serialize to canonical JSON via `FrameGraph::serialize()`.
The payload contains ordered resource declarations, pass metadata (name, queue,
phase, validation severity, read/write lists), and the final execution order.
Stable ordering ensures two graphs with the same declarations produce identical
output, enabling cache keys and deterministic diffing. Serialization throws when
the graph has not been compiled to guard against stale metadata.

## Backend Translation

The Vulkan backend consumes `FrameGraphResourceInfo` instances through
`backend::vulkan::translate_resource()` and barrier metadata via
`translate_barrier()`. Texture descriptors map to `VkImageCreateInfo` (format,
extent, sample count, usage, initial/final layouts) while buffers map to
`VkBufferCreateInfo`. Barrier translation converts generic pipeline stage and
access masks into Vulkan equivalents so the scheduler can emit deterministic
synchronisation without device-specific logic.

Backend adapters can add additional translation layers by relying on the same
schema. The metadata contract therefore remains backend-agnostic while providing
enough fidelity for validation layers and tooling.

## Runtime Integration

`engine::runtime::RuntimeHost` constructs frame-graphs through the same schema:
pass metadata determines queue submission order, and resource descriptors feed
the GPU scheduler when emitting `GpuSubmitInfo`. Runtime integration tests under
`engine/tests/integration/test_runtime_integration.cpp` exercise the
runtime → rendering bridge to ensure resource metadata remains consistent.

## Validation & Tooling

- Unit tests (`engine/rendering/tests/test_frame_graph.cpp`) cover descriptor
  propagation, read/write tracking, and deterministic serialization.
- Vulkan translation tests (`engine/rendering/tests/test_vulkan_resource_translation.cpp`)
  assert that formats, usage flags, and layouts map to the correct Vulkan create
  structures.
- Integration tests ensure runtime submissions use the same metadata contract.
- The serialized JSON is suitable for tooling such as frame capture viewers or
  offline schedulers that rely on a stable schema.
