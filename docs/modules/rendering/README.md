# Rendering Module

## Current State
- Provides a declarative frame graph API for describing render passes, resources, and dependencies, alongside a forward rendering pipeline implementation.
- Implements command encoder abstractions, resource providers (including a recording GPU resource provider), and a material system coordinating shader bindings.
- Defines backend interfaces and scheduler stubs so platform-specific renderers can plug in while sharing common orchestration.
- Ships a Vulkan resource translation layer that converts frame-graph descriptors and synchronization barriers into `VkImage*`
  and `VkBuffer*` create info structures, providing a deterministic bridge between engine metadata and backend allocation.
- Provides a `backend::vulkan::VulkanGpuScheduler` that translates the generic submission stream into Vulkan-native handles; runtime integration tests drive it end-to-end via `RuntimeHost::submit_render_graph`.
- Extensive tests in `engine/rendering/tests/` cover frame graph construction,
  backend adapters, and forward pipeline behaviour, while
  [`engine/tests/integration`](../../../engine/tests/integration/README.md) drives end-to-end
  runtime submissions through the Vulkan scheduler as part of `TI-001`,
  enabled by the Googletest fixture upgrade delivered in `T-0118`.
- Frame graph resources carry explicit format, dimension, usage, and state metadata, and render passes publish queue affinity hints that schedulers consume when selecting submission queues.
- Material and resource descriptors now consume the generational asset handles introduced in the assets module, ensuring rendering references remain valid across cache reloads.

## Usage
- Build via `cmake --build --preset <preset> --target engine_rendering`; this pulls in core, assets, platform, and scene dependencies.
- Include `<engine/rendering/frame_graph.hpp>`, `<engine/rendering/forward_pipeline.hpp>`, or `<engine/rendering/material_system.hpp>` when constructing render workloads.
- Execute `ctest --preset <preset> --tests-regex engine_rendering` to verify frame graph scheduling and pass correctness after changes.

## Frame Graph Metadata and Queue Affinity

- Use `FrameGraphResourceDescriptor` when calling `FrameGraph::create_resource`. Populate:
  - `format` (`ResourceFormat`) and `dimension` (`ResourceDimension`) so GPU resource providers can allocate concrete textures or buffers.
  - `usage` (`ResourceUsage`) to describe how passes will access the resource; combine flags with `operator|` and query them with `has_flag`.
  - `initial_state` / `final_state` (`ResourceState`) to document the expected layout before the first pass and after the last pass that touches the resource.
  - Texture metadata: `width`, `height`, `depth`, `array_layers`, `mip_levels`, and `sample_count`. These drive Vulkan image creation and layout validation.
  - Buffer metadata: `size_bytes` must be provided when `dimension == Buffer` so backend allocators can reserve the correct capacity.
- `FrameGraphResourceInfo` now exposes the same metadata to passes during execution and to `IGpuResourceProvider::on_transient_{acquire,release}`.
- `RenderPass` exposes `set_queue` and an optional constructor parameter so passes can declare their queue affinity; schedulers receive the preferred queue alongside the pass reference when making routing decisions.
- Associate every pass with a `PassPhase` (`Setup`, `Geometry`, `Lighting`, `PostProcess`, `Compute`, `Transfer`, `Presentation`) and a `ValidationSeverity` (`Info`, `Warning`, `Error`). The execution context mirrors these accessors so telemetry and diagnostics can group passes deterministically.
- `engine::rendering::CallbackRenderPass` accepts the preferred queue as its fourth constructor argument for concise setup of compute or transfer passes.
- Call `FrameGraph::serialize()` after compilation to materialise a canonical JSON representation of resources, passes, and execution order. The serializer escapes debug names, preserves declaration order, and ensures repeated invocations yield byte-for-byte identical output for diffing and caching. Serialized resources now include extent, array-layer, mip-count, sample-count, and buffer-size metadata alongside the format/usage/state snapshot.

## Runtime Coordination Responsibilities

- The rendering module owns the authoritative metadata schema for frame-graph resources and pass descriptors. Any change to
  `FrameGraphResourceDescriptor`, pass queue semantics, or serialization fields must be mirrored in the runtime module within the
  same change set.
- Rendering reviewers are responsible for flagging runtime call sites—`RuntimeHost::RenderSubmissionContext`, default pass
  builders, and integration tests in `engine/runtime/tests/`—that need adjustments whenever metadata evolves. Treat the
  `RuntimeHost::submit_render_graph` helpers as API consumers that must compile and pass existing tests after every metadata
  update.
- Runtime contributors must surface new resource requirements (for example additional G-buffer attachments or queue transitions)
  through the rendering module before expanding submission payloads. Document the expectations in task/PR descriptions and
  update both module READMEs so downstream teams understand the split of responsibilities.
- Integration suites under `engine/tests/integration/` act as the shared guardrail: extend or add scenarios whenever metadata
  changes to ensure runtime-generated passes exercise the new descriptors end-to-end through the Vulkan scheduler.

<a id="runtime-rendering-vertical-slice-vulkan-workflow"></a>
## Runtime ↔ Rendering Vertical Slice (Vulkan Workflow)

The Vulkan backend shipped for `RT-003` demonstrates the end-to-end submission path between
`engine::runtime::RuntimeHost` and the rendering frame graph. Follow the checklist below when
exercising or extending the slice; each step maps directly to the types implemented under
[`engine/runtime`](../../../engine/runtime/) and [`engine/rendering`](../../../engine/rendering/).

### Prerequisites

- Build with `ENGINE_ENABLE_RENDERING=ON` so the runtime links against the rendering module and
  default forward pipeline.
- Ensure the runtime dependencies provide a renderable entity. The default
  `RuntimeHostDependencies` installs `rendering::components::RenderGeometry` so the runtime can
  mirror meshes into the scene before submission.
- Register materials whose handles match the runtime geometry descriptors via
  `rendering::MaterialSystem::register_material` before invoking the submission step.

### Assemble the Submission Context

Construct `RuntimeHost::RenderSubmissionContext` (see
[`engine/runtime/api.hpp`](../../../engine/runtime/include/engine/runtime/api.hpp)) immediately
after the runtime advances a frame. Each field has a concrete responsibility during submission:

1. **`rendering::RenderResourceProvider`** – ensures CPU-side asset handles referenced by the
   scene are resident on the GPU. Integration tests ship lightweight reference implementations (see
   [`engine/tests/integration/test_runtime_integration.cpp`](../../../engine/tests/integration/test_runtime_integration.cpp)),
   while production hosts adapt this interface to their streaming/cache layer.
2. **`rendering::MaterialSystem`** – resolves material and shader handles during draw-call
   emission. Populate it with `MaterialRecord` entries whose IDs match the runtime’s
   `RenderGeometry` component.
3. **`rendering::resources::IGpuResourceProvider`** – materialises frame-graph resource metadata.
   For Vulkan, instantiate `resources::RecordingGpuResourceProvider` or a concrete device-backed
   implementation configured with `GraphicsApi::Vulkan` so the scheduler can resolve images,
   buffers, fences, and semaphores.
4. **`rendering::IGpuScheduler`** – translate compiled passes into backend submissions. The Vulkan
   slice uses [`backend::vulkan::VulkanGpuScheduler`](../../../engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp),
   which emits `VulkanSubmission` records containing queue handles, barriers, and timeline semaphore
   payloads.
5. **`rendering::CommandEncoderProvider`** – supplies command encoders bound to the selected queue.
   Tests use `rendering::tests::RecordingCommandEncoderProvider`; production builds provide an
   implementation that records into Vulkan command buffers.
6. **`rendering::FrameGraph`** – receives the passes compiled by the forward pipeline. Reuse a
   persistent instance when driving multiple frames so lifetime tracking retains transient resource
   reuse information.
7. **`rendering::ForwardPipeline` (optional)** – pass a custom pipeline when overriding the default
   forward renderer. When left `nullptr`, `RuntimeHost` invokes its internal
   `ForwardPipeline` instance.

### Submission Flow

Call `RuntimeHost::submit_render_graph(context)` after `RuntimeHost::tick()` completes. The runtime
ensures its render entity exists, chooses the pipeline (custom or default), and delegates to
`ForwardPipeline::render`. The pipeline:

1. Collects visible scene nodes and asks the frame graph to create resources using the descriptor
   metadata defined in `FrameGraphResourceDescriptor`.
2. Invokes the command encoder provider to record draw calls. For the Vulkan slice, the command
   buffer records mesh/material handles that `RenderResourceProvider` resolved earlier.
3. Hands the compiled passes to the Vulkan scheduler. `VulkanGpuScheduler::build_submission`
   resolves native queue and synchronization handles via the GPU resource provider, yielding a
   deterministic `VulkanSubmission` stream.

The integration test
[`engine/tests/integration/test_runtime_integration.cpp`](../../../engine/tests/integration/test_runtime_integration.cpp)
asserts this flow end-to-end: it ticks the runtime, wires the context with recording providers, and
verifies that the scheduler emits a single `ForwardGeometry` submission carrying the expected mesh
and material handles.

### Diagnostics and Regression Guardrails

- Execute `engine_integration_tests` (see
  [`engine/tests/integration/README.md`](../../../engine/tests/integration/README.md)) to confirm the
  vertical slice remains deterministic after modifying either module.
- Capture frame-level timings with `scripts/diagnostics/runtime_frame_telemetry.py`; the script reads
  the same dispatcher metrics surfaced via the Vulkan slice so performance regressions surface in
  dashboards tracked by `RT-003`.
- Mirror any changes to frame-graph metadata or submission ordering into this documentation and the
  runtime README to keep the cross-module contract explicit.

## Vulkan Resource Translation

- `engine::rendering::backend::vulkan::translate_resource` converts a `FrameGraphResourceInfo` into either a `VkImageCreateInfo`
  + `VkImageViewCreateInfo` pair or a `VkBufferCreateInfo`/`VkBufferViewCreateInfo` pair. The function validates that required
  metadata (dimensions, mip levels, buffer size) is provided and maps `ResourceUsage` flags onto Vulkan usage masks.
- `translate_pipeline_stage`, `translate_access_mask`, and `translate_barrier` bridge frame-graph synchronization primitives to
  `VkPipelineStageFlags` and `VkAccessFlags`, ensuring scheduling metadata survives the backend hop.
- The unit test `engine/rendering/tests/test_vulkan_resource_translation.cpp` demonstrates end-to-end translation of color,
  depth-stencil, and buffer resources along with barrier mapping. Use it as a reference when wiring additional backends or
  expanding the Vulkan provider.

## Migration Notes

1. Replace calls to `FrameGraph::create_resource("Name")` with `FrameGraph::create_resource(descriptor)` and populate all descriptor fields for transient resources.
2. Update custom render passes to either forward the preferred queue through the base-class constructor or call `set_queue` inside their constructor.
3. Extend tests to assert the propagated metadata using `FrameGraph::resource_info`, `FrameGraphPassExecutionContext::pass_phase`, and the deterministic serializer to catch regressions when adding new passes.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
