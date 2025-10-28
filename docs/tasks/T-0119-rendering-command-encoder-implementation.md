# T-0119: Rendering Command Encoder Implementation
**Status**: 🟡 In Progress — OpenGL command encoder now records geometry draws and integrates with the GPU scheduler. Backend-specific API submissions remain outstanding.
## Goal
Implement concrete command encoder backends for Vulkan, DirectX12, Metal, and OpenGL to translate high-level draw commands into actual GPU API calls.
## Background
- Roadmap alignment: Critical for `RT-003` (Vulkan) and future backend support.
- Current state: `CommandEncoder` interface exists with `draw_geometry()` method, but no concrete implementations exist. Frame-graph can schedule work but cannot encode actual GPU commands.
- Dependencies: Requires GPU resource provider implementation (T-0120) to access actual buffers, textures, and pipeline state objects.
## Inputs
- Code: `engine/rendering/include/engine/rendering/command_encoder.hpp`, `engine/rendering/src/forward_pipeline.cpp`
- Backend headers: `engine/rendering/include/engine/rendering/backend/vulkan/`, `engine/rendering/include/engine/rendering/backend/directx12/`, etc.
- Existing scheduler implementations: `VulkanGpuScheduler`, `DirectX12GpuScheduler`, `MetalGpuScheduler`, `OpenGLGpuScheduler`
## Constraints
- Backend-agnostic interface: No API-specific types should leak into the abstract `CommandEncoder` interface.
- Must work with frame-graph command buffer allocation and recycling.
- Support all geometry types: meshes, graphs, and point clouds.
- Handle material/shader binding per draw call.
- Properly encode render pass begin/end, pipeline binding, descriptor sets, and draw commands.
## Deliverables
### 1. Vulkan Command Encoder (`VulkanCommandEncoder`)
- **File**: `engine/rendering/src/backend/vulkan/command_encoder.cpp` + header
- **Responsibilities**:
  - Begin/end render pass with color/depth attachments from frame-graph resources
  - Bind graphics pipeline based on material shader
  - Bind vertex/index buffers from mesh handles
  - Update push constants or descriptor sets with transform matrices
  - Issue `vkCmdDrawIndexed` or `vkCmdDraw` calls
  - Handle viewport/scissor state
- **Integration**: Works with `VulkanGpuScheduler` and resource provider
### 2. DirectX12 Command Encoder (`DirectX12CommandEncoder`)
- **File**: `engine/rendering/src/backend/directx12/command_encoder.cpp` + header
- **Responsibilities**:
  - Set render targets from frame-graph resources
  - Bind graphics pipeline state object (PSO)
  - Set root signature and constant buffers for transforms
  - Bind vertex/index buffer views
  - Issue `DrawIndexedInstanced` or `DrawInstanced` commands
  - Handle viewport/scissor rectangles
- **Integration**: Works with `DirectX12GpuScheduler`
### 3. Metal Command Encoder (`MetalCommandEncoder`)
- **File**: `engine/rendering/src/backend/metal/command_encoder.cpp` + header
- **Responsibilities**:
  - Create render command encoder with render pass descriptor
  - Set render pipeline state
  - Set vertex buffers and index buffers
  - Set vertex/fragment shader arguments for transforms/materials
  - Issue `drawIndexedPrimitives` or `drawPrimitives` commands
  - End encoding
- **Integration**: Works with `MetalGpuScheduler`
### 4. OpenGL Command Encoder (`OpenGLCommandEncoder`)
- **File**: `engine/rendering/src/backend/opengl/command_encoder.cpp` + header
- **Responsibilities**:
  - Bind framebuffer object for render targets
  - Use shader program from material
  - Bind vertex array object (VAO) from mesh
  - Set uniform values for transforms/materials
  - Issue `glDrawElements` or `glDrawArrays` calls
  - Handle viewport state
- **Integration**: Works with `OpenGLGpuScheduler`
### 5. Command Encoder Provider Implementation
- **File**: `engine/rendering/src/backend/command_encoder_provider.cpp` + header
- **Responsibilities**:
  - Factory pattern to create appropriate encoder based on active backend
  - Manage encoder lifecycle (begin/end)
  - Cache encoders per command buffer if needed
- **API**: Implements `CommandEncoderProvider` interface
### 6. Testing
- Unit tests for each backend encoder (mock GPU objects)
- Integration tests with actual frame-graph execution
- Verify draw commands are correctly encoded
- Test geometry type variants (mesh, graph, point cloud)
- Test material binding and shader parameter updates
### 7. Documentation
- Document command encoder architecture in `docs/modules/rendering/command-encoding.md`
- Add backend-specific encoding details
- Provide examples of custom render pass implementation
## Work Breakdown
1. Implement `VulkanCommandEncoder` (priority: high)
2. Implement `OpenGLCommandEncoder` (priority: medium)
3. Implement `DirectX12CommandEncoder` (priority: medium)
4. Implement `MetalCommandEncoder` (priority: low)
5. Implement `CommandEncoderProvider` factory
6. Add comprehensive tests
7. Document architecture and usage patterns
## Acceptance Criteria
- [ ] `VulkanCommandEncoder` translates `draw_geometry()` into valid Vulkan command buffer recording
- [ ] `OpenGLCommandEncoder` translates `draw_geometry()` into valid OpenGL state and draw calls
- [ ] `DirectX12CommandEncoder` translates `draw_geometry()` into valid D3D12 command list recording
- [ ] `MetalCommandEncoder` translates `draw_geometry()` into valid Metal render commands
- [ ] `CommandEncoderProvider` correctly instantiates backend-specific encoders
- [ ] Tests verify correct command encoding for all geometry types
- [ ] Integration test shows full render loop with actual GPU submission (backend-dependent)
- [ ] Documentation covers architecture and extension points
## Metrics & Benchmarks
- Command encoding overhead < 100ns per draw call
- Memory allocation minimal during encoding (use pre-allocated pools)
- Validation layers report zero errors for generated commands
## Follow-Up
- Add support for compute shader dispatch in command encoders
- Implement indirect draw command support
- Add multi-draw batching optimization
- Implement ray tracing command encoding (DXR/Vulkan RT)
## Open Questions
- Should we cache pipeline state objects or create them on-demand?
- How do we handle descriptor set allocation and recycling?
- What's the strategy for push constants vs. descriptor sets in Vulkan?
- Should we support immediate-mode rendering fallback for debugging?

## Progress Log
- 2025-12-08: Introduced the `OpenGLCommandEncoder` and a provider that records geometry draws into command buffer submissions.
  Scheduler submissions now surface recorded draw lists, enabling validation ahead of backend-specific API bindings.
- 2025-12-12: Added `OpenGLImmediateCommandStream` which resolves meshes through the render resource provider and issues
  OpenGL draw calls (with draw-count instrumentation for headless runs), wiring recorded geometry directly into the command
  stream for AI-004 prototypes.
- 2025-12-13: Introduced `OpenGLRuntimeSubmission`, bundling the render resource provider, command stream, GPU resource
  provider, encoder provider, and scheduler so runtime hosts can execute frame graphs with real draw submissions backed by the
  OpenGL command encoder.
