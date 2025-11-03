# Rendering Module

## Overview

> **Status:** ⚠️ **Blocked** — GPU resource provider and command encoder work (see [`T-0120`](../../backlog/active/T-0120-gpu-resource-provider.md) and [`T-0119`](../../backlog/active/T-0119-command-encoder-integration.md)) remain unfinished. OpenGL and Vulkan command encoders now record frame-graph draw/dispatch commands for scheduler inspection, but the backends still rely on the recording provider and cannot allocate GPU buffers, textures, or shaders, so no real draw commands execute yet.

The rendering module currently provides frame-graph compilation, scheduler prototypes, and resource lifetime tracking, but the missing GPU execution path prevents end-to-end rendering. This README tracks the outstanding work needed to reach functional backends in addition to describing the existing infrastructure.

## Outstanding Work

- Implement the GPU resource provider (`T-0120`) to create buffers, textures, and shader programs for real backends.
- Land the command encoder integration (`T-0119`) so frame-graph passes emit backend command buffers.
- Coordinate with the runtime stage planner (`RT-410`) to ensure presentation backends and synchronisation policies align with rendering.

## Camera System

`engine/rendering/camera.hpp` exposes a lightweight `rendering::Camera` struct that tracks model, view, and projection matrices in lockstep. Helper methods build view matrices via `look_at`, configure common perspective/orthographic projections, and convert to/from `math::Transform` instances so scene systems can synchronise world transforms with GPU uploads. Controllers under `engine/rendering/camera_controllers.hpp` wrap the camera by reference, providing reusable update policies for first-person and orbit navigation styles. Both controllers accept `CameraControlState` input—translation, rotation, and zoom deltas—allowing UI or input layers to share a consistent integration surface while keeping ownership with the rendering module.

## Frame Graphs

### Frame Graph Definition

Define rendering passes and resources declaratively:

```cpp
#include "engine/rendering/frame_graph.hpp"

rendering::FrameGraphBuilder builder;

// Declare resources
auto color_buffer = builder.create_texture({
    .width = 1920,
    .height = 1080,
    .format = rendering::TextureFormat::RGBA8,
    .usage = rendering::TextureUsage::RenderTarget
});

auto depth_buffer = builder.create_texture({
    .width = 1920,
    .height = 1080,
    .format = rendering::TextureFormat::Depth24Stencil8,
    .usage = rendering::TextureUsage::DepthStencil
});

// Add render pass
builder.add_pass("geometry_pass")
    .writes(color_buffer)
    .writes(depth_buffer)
    .execute([](rendering::RenderPassContext& ctx) {
        // Record draw commands
        ctx.bind_pipeline(geometry_pipeline);
        ctx.draw(mesh);
    });

// Compile frame graph
auto frame_graph = builder.compile();
```

> **Resource naming**
>
> Frame-graph resources must be declared with unique, non-empty names. The builder throws
> `std::invalid_argument` if a descriptor omits a name or attempts to reuse an existing identifier
> so telemetry, serialization, and diagnostics remain unambiguous.

### Frame Graph Execution

Execute the compiled graph through a scheduler:

```cpp
rendering::GpuScheduler scheduler{rendering::Backend::Vulkan};

// Execute frame graph
rendering::ExecutionResult result = scheduler.execute(frame_graph);

if (result.success) {
    fmt::print("Frame rendered in {:.3f}ms\n", result.duration_ms);
} else {
    fmt::print("Render failed: {}\n", result.error_message);
}
```

### Resource Barriers

The frame graph automatically inserts barriers:

```cpp
// Pass A writes to texture
builder.add_pass("pass_a").writes(texture);

// Pass B reads from texture
builder.add_pass("pass_b").reads(texture);

// Automatic barrier inserted between passes
// No manual synchronization required
```

## Research Baseline Preset

The research initiative behind `RE-610` introduces a reusable frame-graph preset
that mirrors the lighting and debugging requirements for prototyping work. Use
`rendering::configure_research_baseline` to populate a `FrameGraph` with a
forward or deferred pipeline and optional debug overlays:

```cpp
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"

rendering::FrameGraph graph;
rendering::ResearchBaselineOptions options{};
options.shading_mode = rendering::ResearchShadingMode::Deferred;
options.enable_normals_overlay = true;

const auto resources = rendering::configure_research_baseline(graph, options);
graph.compile();
```

The helper creates the following passes in order:

| Stage | Pass Name | Description |
|-------|-----------|-------------|
| Geometry | `Research.ForwardGeometry` / `Research.GBuffer` | Populates the primary color target or deferred G-Buffer attachments. |
| Lighting | `Research.LightingComposite` | Deferred-only lighting resolve into `Research.FinalColor`. |
| Debug Overlays | `Research.Debug.*` | Optional passes writing normals/UV/material/light-volume overlays when enabled. |

Resource handles returned from the helper expose the final color target, depth
buffer, G-Buffer attachments, and overlay outputs so the runtime harness can
bind them to presentation or telemetry capture paths. Default dimensions are
1920×1080, but callers can override them through `ResearchBaselineOptions`.

### Telemetry

`ResearchBaselineTelemetry` records metrics for diagnostics and runtime
dashboards whenever the preset is configured or executed:

- `rendering.research.shading_mode.selection` (counter) — increments each time
  the preset is configured for a shading variant. Label `mode` is either
  `forward` or `deferred`.
- `rendering.research.shading_mode.active` (gauge) — reports the currently
  active shading mode as `0` for forward and `1` for deferred pipelines.
- `rendering.research.overlay.selection` (counter) — increments whenever a debug
  overlay is enabled during configuration. Label `overlay` is one of
  `normals`, `uv`, `material`, or `light_volume`.
- `rendering.research.overlay.enabled` (gauge) — reports the active state of
  each overlay as `1` when enabled and `0` when disabled. Shares the `overlay`
  label values listed above.
- `rendering.research.pass.draw_calls_total` (counter) — cumulative draw calls
  submitted by each pass. Labels: `pass` and `phase` (frame-graph phase).
- `rendering.research.pass.last_draw_calls` (gauge, unit Count) — draw calls
  submitted during the most recent execution of a pass.
- `rendering.research.pass.last_gpu_time_ms` (gauge, unit Milliseconds) — most
  recent execution time per pass measured while encoding GPU work.
- `rendering.research.pass.max_gpu_time_ms` (gauge, unit Milliseconds) — peak
  execution time observed for each pass since the telemetry state was reset.
- Runtime diagnostics also publish GPU resource usage gauges
  (`rendering.resources.buffer_bytes`, `rendering.resources.texture_bytes`,
  `rendering.resources.other_bytes`, `rendering.resources.total_bytes`) so
  PM-510 demos can correlate buffer and texture residency with frame-graph
  activity once GPU providers are active.

Runtime diagnostics export these metrics through the shared telemetry schema so
the prototyping harness and benchmarking automation can surface draw-call
counts, pass timings, and shading-mode usage without additional wiring.

### AI-004 Configuration Migration

- Rendering presets referenced by the prototyping harness must declare the
  `ai-004.rendering` schema block before strict validation is enabled. The
  repository ships `docs/examples/ai004_sample.json`, which already embeds the
  research baseline preset alongside the sample remeshing dataset. Use the
  shared validator:
  ```bash
  python -m scripts.validate_ai004_config --config docs/examples/ai004_sample.json
  ```
  to surface missing headers or invalid preset names.
- During the migration window, `python -m scripts.prototyping.run_prototype_harness`
  can be executed with `--require-schema` (or by exporting
  `ENGINE_AI004_SCHEMA_V1=1`) to guarantee updated presets are consumed by the
  runtime harness before frame-graph execution.
- Document any module-specific overrides or experimental passes inside the
  configuration manifest so downstream tools inherit the same metadata when the
  schema check becomes mandatory.

## Backend Support

### Vulkan Backend

```cpp
rendering::VulkanBackendConfig vulkan_config{
    .device_index = 0,
    .enable_validation = true,
    .enable_debug_markers = true
};

rendering::GpuScheduler scheduler{
    rendering::Backend::Vulkan,
    vulkan_config
};
```

See [`BACKEND_CHECKLIST.md`](BACKEND_CHECKLIST.md) for Vulkan implementation status.

The module now ships `backend::vulkan::VulkanGpuResourceProvider`, which
generates deterministic Vulkan queue, command buffer, and transient resource
handles so scheduler and command-encoder work can progress without binding to a
physical device. Pair it with the Vulkan scheduler in offline tests to exercise
retention policies and telemetry hooks before wiring real driver allocations.

### DirectX 12 Backend (Planned)

```cpp
rendering::DirectX12Config dx12_config{
    .adapter_index = 0,
    .enable_pix_markers = true
};

rendering::GpuScheduler scheduler{
    rendering::Backend::DirectX12,
    dx12_config
};
```

### OpenGL Backend (Queue-Normalised)

```cpp
rendering::resources::RecordingGpuResourceProvider provider(rendering::resources::GraphicsApi::OpenGL);
rendering::backend::opengl::OpenGLGpuScheduler scheduler{provider};

// Constructing the scheduler with a provider that does not report
// GraphicsApi::OpenGL throws std::invalid_argument.

auto command_buffer = scheduler.request_command_buffer(rendering::QueueType::Compute, "LightingPass");

rendering::GpuSubmitInfo submit{};
submit.pass_name = "LightingPass";
submit.queue = rendering::QueueType::Graphics;  // non-graphics queues are normalised
submit.command_buffer = command_buffer;
submit.begin_barriers.push_back({
    .source_stage = rendering::resources::PipelineStage::Compute,
    .source_access = rendering::resources::Access::Write,
    .destination_stage = rendering::resources::PipelineStage::Graphics,
    .destination_access = rendering::resources::Access::Read,
});

scheduler.submit(submit);

const auto& submission = scheduler.submissions().front();
const auto begin_mask = submission.begin_memory_barrier_mask();
const auto end_mask = submission.end_memory_barrier_mask();
for (const auto& barrier : submission.begin_barriers)
{
    fmt::print("Barrier mask: 0x{:04x}\n", barrier.memory_barrier_mask);
}
```

- `OpenGLGpuScheduler::select_queue` coerces compute/transfer queues to `Graphics` to match the single-queue OpenGL command
  stream while preserving deterministic scheduling.
- Each submission captures translated barrier metadata via `OpenGLBarrier::memory_barrier_mask`, combining the required
  `glMemoryBarrier` bits for both the source and destination pipeline stages, and exposes aggregated masks through
  `OpenGLSubmission::begin_memory_barrier_mask()` / `end_memory_barrier_mask()` for diagnostics.
- Barrier translation constants (`backend::opengl::shader_image_access_barrier_bit`,
  `backend::opengl::texture_update_barrier_bit`, …) are exposed for diagnostics and tests.
- Backend adapter tests (`BackendAdapters.OpenGLSchedulerRecordsGraphicsQueue` and
  `BackendAdapters.OpenGLSchedulerNormalisesQueueSelections`) validate queue selection, barrier translation, and semaphore
  resolution.
- The scheduler routes translated submissions through an overridable command stream interface. Provide a custom
  `backend::opengl::CommandStream` implementation when instrumentation or driver integration is required:

  ```cpp
  class LoggingStream final : public rendering::backend::opengl::CommandStream {
  public:
      void begin_submission(const rendering::backend::opengl::OpenGLSubmission& submission) override {
          fmt::print("Begin {}\n", submission.pass_name);
      }
      // Override the remaining virtual members as needed…
  };

  LoggingStream stream;
  rendering::backend::opengl::OpenGLGpuScheduler scheduler(provider, &stream);
  ```

- When no custom stream is supplied the default implementation issues `glMemoryBarrier` calls (when GLAD is available) for the
  aggregated masks and flushes the queue, allowing headless test harnesses to observe deterministic ordering without requiring
  a live OpenGL context.
- The module now ships `rendering::backend::opengl::OpenGLImmediateCommandStream`, a concrete command stream that resolves
  mesh handles through `OpenGLRenderResourceProvider` and issues `glDraw*` calls when GLAD is present.  The stream also counts
  attempted draw calls so tests can validate behaviour without a real OpenGL context:

  ```cpp
  rendering::backend::opengl::OpenGLRenderResourceProvider render_resources(mesh_resolver);
  rendering::backend::opengl::OpenGLImmediateCommandStream stream(render_resources);
  rendering::backend::opengl::OpenGLGpuScheduler scheduler(provider, &stream);
  ```

- For runtime hosts and samples that need a pre-wired submission stack, use
  `rendering::backend::opengl::OpenGLRuntimeSubmission`. The adapter owns the
  render resource provider, immediate command stream, GPU resource provider,
  command encoder provider, and scheduler, exposing a convenience helper for
  constructing `RuntimeSubmissionContext` instances without threading each
  dependency manually.
- `rendering::backend::opengl::OpenGLPresentationBackend` builds on the runtime
  submission adapter by implementing the `rendering::PresentationBackend`
  interface. Provide a mesh resolver callback and register materials with the
  embedded `MaterialSystem`, then call `present()` with the runtime's
  `RuntimePresentationContext` (which carries a `submit_render_graph` callback)
  to execute the frame graph through the OpenGL stack each tick. Configure
  transient resource reuse by supplying a retention-frame count to the
  constructor (or by calling `set_resource_retention_frames()`), enabling
  runtime hosts and PM-510 demos to balance GPU memory pressure against reuse
  while T-0120 backends mature.

## Resource Management

### Texture Resources

```cpp
rendering::TextureDescriptor desc{
    .width = 1024,
    .height = 1024,
    .format = rendering::TextureFormat::RGBA8,
    .mip_levels = 4,
    .usage = rendering::TextureUsage::Sampled | rendering::TextureUsage::TransferDst
};

auto texture = resource_provider.create_texture(desc);

// Upload data
resource_provider.upload_texture(texture, pixel_data);

// Generate mipmaps
resource_provider.generate_mipmaps(texture);
```

### Buffer Resources

```cpp
rendering::BufferDescriptor desc{
    .size = vertex_count * sizeof(Vertex),
    .usage = rendering::BufferUsage::VertexBuffer,
    .memory_type = rendering::MemoryType::DeviceLocal
};

auto buffer = resource_provider.create_buffer(desc);
resource_provider.upload_buffer(buffer, vertex_data);
```

### Resource Lifetime

Resources use handle-based lifetime management (from `AI-001`):

```cpp
// Acquire resource handle
auto handle = resource_provider.create_texture(desc);

// Use resource
render_pass.bind_texture(handle, binding);

// Release handle (decrements reference count)
resource_provider.release(handle);

// Resource freed when last handle released
```

### Resource Events

Track resource lifecycle through events:

```cpp
const auto& diag = runtime::diagnostics();
const auto& events = diag.frame_graph_events;

for (const auto& event : events) {
    fmt::print("{}: {} at frame {}\n",
        event.resource_name,
        rendering::event_type_name(event.type),
        event.frame_number);
}
```

Event types:
- `Created`: Resource allocated
- `Destroyed`: Resource freed
- `Transitioned`: Layout/state transition
- `Aliased`: Memory aliased with another resource

## Pipeline State

### Graphics Pipelines

```cpp
rendering::GraphicsPipelineDescriptor desc{
    .vertex_shader = "shaders/vertex.spv",
    .fragment_shader = "shaders/fragment.spv",
    .vertex_format = {
        {rendering::VertexAttribute::Position, rendering::Format::RGB32Float},
        {rendering::VertexAttribute::Normal, rendering::Format::RGB32Float},
        {rendering::VertexAttribute::UV, rendering::Format::RG32Float}
    },
    .rasterizer_state = {
        .cull_mode = rendering::CullMode::Back,
        .front_face = rendering::FrontFace::CounterClockwise
    },
    .depth_state = {
        .depth_test_enable = true,
        .depth_write_enable = true,
        .depth_compare_op = rendering::CompareOp::Less
    },
    .blend_state = {
        .blend_enable = false
    }
};

auto pipeline = resource_provider.create_graphics_pipeline(desc);
```

### Compute Pipelines

```cpp
rendering::ComputePipelineDescriptor desc{
    .compute_shader = "shaders/particle_update.comp.spv",
    .push_constant_size = sizeof(ParticleParams)
};

auto pipeline = resource_provider.create_compute_pipeline(desc);
```

## Command Recording

### Render Pass Commands

```cpp
pass_context.begin_render_pass(render_pass_handle);

pass_context.bind_pipeline(graphics_pipeline);
pass_context.bind_vertex_buffer(vertex_buffer, 0);
pass_context.bind_index_buffer(index_buffer, rendering::IndexType::UInt32);
pass_context.bind_descriptor_set(descriptor_set, 0);

pass_context.set_viewport({0, 0, width, height});
pass_context.set_scissor({0, 0, width, height});

pass_context.draw_indexed(index_count, 1, 0, 0, 0);

pass_context.end_render_pass();
```

### Compute Dispatch

```cpp
compute_context.bind_compute_pipeline(compute_pipeline);
compute_context.bind_descriptor_set(descriptor_set, 0);
compute_context.push_constants(&params, sizeof(params));
compute_context.dispatch(workgroup_x, workgroup_y, workgroup_z);
```

## Runtime Integration

The runtime submits frame graphs through the rendering module:

```cpp
#if ENGINE_ENABLE_RENDERING
rendering::RuntimeSubmissionContext context{
    .scheduler = scheduler,
    .backend = rendering::Backend::Vulkan,
    .frame_number = frame_count
};

runtime_host.submit_render_graph(context);
#endif
```

Frame graph metadata is captured in diagnostics:

```cpp
const auto& diag = runtime::diagnostics();
const auto& graph_json = diag.frame_graph_serialization;
// JSON representation of frame graph structure
```

## Validation

Enable validation layers for debugging:

```cpp
rendering::ValidationConfig validation{
    .enable_api_validation = true,
    .enable_synchronization_validation = true,
    .enable_thread_safety_validation = true
};

scheduler.set_validation_config(validation);
```

Validation checks:
- Resource barrier correctness
- Pipeline state consistency
- Descriptor binding validity
- Memory access patterns
- Synchronization primitives

## Telemetry

Rendering operations emit comprehensive telemetry:

```cpp
const auto& backend_metrics = scheduler.metrics();

fmt::print("Draw calls: {}\n", backend_metrics.draw_call_count);
fmt::print("Triangles: {}\n", backend_metrics.triangle_count);
fmt::print("Texture bindings: {}\n", backend_metrics.texture_bind_count);
fmt::print("Pipeline switches: {}\n", backend_metrics.pipeline_switch_count);
fmt::print("GPU time: {:.3f}ms\n", backend_metrics.gpu_time_ms);
```

Metrics align with the schema in [`METADATA_SCHEMA.md`](METADATA_SCHEMA.md).

## Performance Considerations

Frame graph compilation is deterministic and cached:
- Compilation: ~0.5ms for typical graph
- Execution overhead: ~0.1ms per frame
- Resource barriers: Automatically optimized

Vulkan backend benchmarks (from `T-0116`):
- Simple scene: ~2.0ms GPU time
- Complex scene (10k draw calls): ~8.5ms GPU time
- Resource transition overhead: ~0.03ms per barrier

## Testing

Rendering tests validate:
- Frame graph compilation (`test_frame_graph.cpp`)
- Backend parity (`test_backends.cpp`)
- Resource lifetime tracking (`test_resources.cpp`)
- Command recording (`test_commands.cpp`)
- Vulkan integration (`test_vulkan.cpp`)
- Runtime integration with OpenGL queue normalisation (`engine/runtime/tests/test_module.cpp` → `RuntimeHost.SubmitsRenderGraphThroughOpenGLScheduler`)

Run tests:
```bash
ctest --preset linux-gcc-debug -R rendering
```

## Dependencies

- **Platform**: Window surface creation
- **Math**: Transform matrices, projection matrices
- **Core**: Telemetry schema, error handling
- **Assets** (optional): Texture and shader loading
- **Vulkan SDK** (optional): Required for Vulkan backend

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones including backend parity work
- [`BACKEND_CHECKLIST.md`](BACKEND_CHECKLIST.md): Backend implementation status
- [`METADATA_SCHEMA.md`](METADATA_SCHEMA.md): Frame graph metadata specification
- [`../../specs/ADR-0003-runtime-frame-graph.md`](../../specs/ADR-0003-runtime-frame-graph.md): Frame graph architecture
- [`../../specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md): Presentation abstraction and UI compositing requirements.
- [`../../archive/backlog/legacy/tasks/T-0104-runtime-frame-graph-integration.md`](../../archive/backlog/legacy/tasks/T-0104-runtime-frame-graph-integration.md): Integration milestone
- [`../../archive/backlog/legacy/tasks/T-0116-rendering-vulkan-resource-translation.md`](../../archive/backlog/legacy/tasks/T-0116-rendering-vulkan-resource-translation.md): Vulkan implementation

## Current State

- Frame-graph compilation/execution, command encoder hooks (now capturing both geometry draws and compute dispatches), resource lifetime tracking, Vulkan scheduler prototype, and OpenGL scheduler queue-normalisation with translated `glMemoryBarrier` masks. Backend validation metrics cover all providers and consume the shared metadata schema aligned with runtime submission invariants.
- OpenGL GPU resource provider materialises transient frame-graph textures on acquire, reusing allocations when descriptors
  match, tagging depth attachments so framebuffer wiring can bind native handles once command encoding lands, and recording
  acquire/release metadata for telemetry consumers. Unused buffers and textures are now evicted automatically when they remain
  idle across consecutive frames so transient caches cannot leak driver resources while retaining short-term reuse, the
  retention window is configurable so harnesses and tests can balance reuse against memory pressure, and aggregate byte
  counters expose the live buffer/texture footprint for backend telemetry.
- Runtime-facing OpenGL render resource provider resolves mesh handles into CPU vertex/index buffers and precomputes
  normals/UV data, uploading them to GPU buffers when OpenGL is available so command encoding can bind ready-to-use geometry
  assets.

## Usage

- Run rendering tests (when enabled):
  - `ctest --preset linux-gcc-debug -R rendering`
- See `engine/rendering/tests/` for frame-graph and resource lifetime examples.

## TODO / Next Steps

- Deliver [`T-0120`](../../backlog/active/T-0120-gpu-resource-provider.md): finish GPU buffer/texture/sampler creation, shader compilation, and hot-reload hooks so OpenGL/Vulkan providers allocate real resources. Track progress in [`../../ROADMAP.md`](../../ROADMAP.md).
- Complete [`T-0119`](../../backlog/active/T-0119-command-encoder-integration.md): land command encoder APIs, backend submissions, and smoke coverage that translate frame-graph passes into GPU work.
- Coordinate weekly with runtime/tools leads while `T-0120`/`T-0119` progress to align telemetry expectations and unblock downstream [`RT-410`](../../backlog/active/RT-410-runtime-stage-planner.md) integration.
