# Rendering Module

## Overview

The rendering module provides frame-graph compilation and execution, command encoder hooks, GPU resource lifetime tracking, and multi-backend support (Vulkan, DirectX, OpenGL). It implements deterministic scheduling with comprehensive validation and telemetry integration.

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

See [`backend_checklist.md`](backend_checklist.md) for Vulkan implementation status.

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

### OpenGL Backend (Legacy)

```cpp
rendering::GpuScheduler scheduler{rendering::Backend::OpenGL};
// Simplified API for legacy support
```

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

Metrics align with the schema in [`metadata_schema.md`](metadata_schema.md).

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

Run tests:
```bash
ctest --preset clang-debug -R rendering
```

## Dependencies

- **Platform**: Window surface creation
- **Math**: Transform matrices, projection matrices
- **Core**: Telemetry schema, error handling
- **Assets** (optional): Texture and shader loading
- **Vulkan SDK** (optional): Required for Vulkan backend

## Related Documentation

- [`ROADMAP.md`](ROADMAP.md): Module milestones including backend parity work
- [`backend_checklist.md`](backend_checklist.md): Backend implementation status
- [`metadata_schema.md`](metadata_schema.md): Frame graph metadata specification
- [`../../specs/ADR-0003-runtime-frame-graph.md`](../../specs/ADR-0003-runtime-frame-graph.md): Frame graph architecture
- [`../../tasks/T-0104-runtime-frame-graph-integration.md`](../../tasks/T-0104-runtime-frame-graph-integration.md): Integration milestone
- [`../../tasks/T-0116-rendering-vulkan-resource-translation.md`](../../tasks/T-0116-rendering-vulkan-resource-translation.md): Vulkan implementation


