# T-0120: GPU Resource Provider Implementation
**Status**: 🟡 In Progress — OpenGL provider now allocates and tracks command buffers, fences, and timeline handles while real GPU resource creation remains outstanding.

## Goal
Implement concrete `IGpuResourceProvider` backends that create, manage, and track actual GPU resources (buffers, textures, pipelines) for each graphics API.

## Background
- Roadmap alignment: Foundation for `RT-003` (Vulkan backend) and all rendering functionality.
- Current state: `IGpuResourceProvider` interface exists, `RecordingGpuResourceProvider` only records calls without executing them. No actual GPU memory allocation or resource creation.
- Dependencies: Platform integration for device/context creation; asset loading for mesh/texture data.

## Inputs
- Code: `engine/rendering/include/engine/rendering/resources/resource_provider.hpp`
- Backend infrastructure: Device initialization, memory allocators (VMA for Vulkan)
- Asset system: `engine/assets/` for mesh, texture, material loading

## Constraints
- Must integrate with frame-graph transient resource lifecycle
- Support both transient (temporary) and persistent (external) resources
- Implement proper memory management and synchronization
- No resource leaks; track all allocations
- Backend-agnostic API surface; implementation details hidden

## Deliverables

### 1. Vulkan Resource Provider (`VulkanGpuResourceProvider`)
- **File**: `engine/rendering/src/backend/vulkan/resource_provider.cpp` + header
- **Responsibilities**:
  - Device/physical device/queue family management
  - Memory allocation via VMA (Vulkan Memory Allocator)
  - Buffer creation: vertex, index, uniform, storage buffers
  - Image creation: textures, render targets, depth buffers
  - Image view and sampler management
  - Pipeline state object creation and caching
  - Descriptor set layout and pool management
  - Command buffer allocation from pools
  - Fence and semaphore creation
  - Transient resource pooling and recycling
  - Frame-graph resource lifecycle callbacks

### 2. DirectX12 Resource Provider (`DirectX12GpuResourceProvider`)
- **File**: `engine/rendering/src/backend/directx12/resource_provider.cpp` + header
- **Responsibilities**:
  - ID3D12Device management
  - D3D12MA allocator integration
  - Committed/placed resource creation
  - Descriptor heap management (CBV/SRV/UAV, RTV, DSV)
  - Root signature creation and caching
  - PSO creation and caching
  - Command allocator pools
  - Fence synchronization primitives
  - Resource state tracking
  - Transient resource aliasing

### 3. Metal Resource Provider (`MetalGpuResourceProvider`)
- **File**: `engine/rendering/src/backend/metal/resource_provider.cpp` + header
- **Responsibilities**:
  - MTLDevice management
  - Buffer allocation (shared/private/managed modes)
  - Texture creation with MTLTextureDescriptor
  - Render pipeline state creation
  - Depth/stencil state management
  - Sampler state caching
  - Command buffer management from command queue
  - Event/fence synchronization
  - Heap-based memory management for transients

### 4. OpenGL Resource Provider (`OpenGLGpuResourceProvider`)
- **File**: `engine/rendering/src/backend/opengl/resource_provider.cpp` + header
- **Responsibilities**:
  - Context management (via platform layer)
  - Buffer object (VBO/IBO/UBO) creation
  - Texture object creation and binding
  - Framebuffer object (FBO) management for render targets
  - Vertex array object (VAO) creation
  - Shader program compilation and linking
  - Sync object (fence) management
  - Resource cleanup and garbage collection

### 5. Asset-to-GPU Upload Pipeline
- **File**: `engine/rendering/src/resources/upload_manager.cpp` + header
- **Responsibilities**:
  - Staging buffer management for CPU-to-GPU transfers
  - Asynchronous upload queue
  - Texture mipmap generation
  - Buffer data packing and alignment
  - Upload completion tracking

### 6. Resource Cache and Tracking
- **File**: `engine/rendering/src/resources/resource_cache.cpp` + header
- **Responsibilities**:
  - Track loaded meshes, textures, materials by handle
  - Reference counting for shared resources
  - LRU eviction policy for memory management
  - Query residency status
  - Support hot-reloading

### 7. Frame-Graph Resource Integration
- Implement `on_transient_acquire()` - allocate or reuse pooled resource
- Implement `on_transient_release()` - return resource to pool
- Ensure resources match frame-graph descriptors exactly

### 8. Testing
- Unit tests for resource creation/destruction per backend
- Test buffer/texture upload pipeline
- Test transient resource pooling and recycling
- Verify no memory leaks with valgrind/ASAN
- Stress test with rapid allocation/deallocation
- Integration test with frame-graph execution

### 9. Documentation
- Document resource provider architecture in `docs/modules/rendering/resource-management.md`
- Backend-specific allocation strategies
- Memory budget recommendations
- Debugging leaked resources

## Work Breakdown
1. Implement Vulkan resource provider with VMA integration (priority: critical)
2. Implement upload manager for staging transfers
3. Implement resource cache and tracking
4. Integrate with frame-graph transient lifecycle
5. Implement OpenGL resource provider (priority: high)
6. Implement DirectX12 resource provider (priority: medium)
7. Implement Metal resource provider (priority: low)
8. Add comprehensive testing suite
9. Document architecture and best practices

## Acceptance Criteria
- [ ] `VulkanGpuResourceProvider` creates actual Vulkan buffers, images, and pipelines
- [ ] Resource provider correctly handles frame-graph transient acquire/release
- [ ] Upload manager transfers mesh/texture data from CPU to GPU
- [ ] Resource cache prevents duplicate allocations for same asset handle
- [ ] No memory leaks detected in 1000-frame stress test
- [ ] All backends implement full `IGpuResourceProvider` interface
- [ ] Integration test: frame-graph executes with real GPU resources
- [ ] Documentation covers allocation strategies and troubleshooting

## Metrics & Benchmarks
- Transient resource pool reduces allocations by >80% compared to naive approach
- Upload throughput: >500 MB/s for texture streaming
- Resource cache hit rate >90% in typical scenes
- Maximum memory overhead <10% of working set
- Resource creation time: <1ms for typical assets

## Follow-Up
- Implement bindless descriptor model for modern APIs
- Add support for sparse resources and virtual textures
- Implement resource streaming and priority management
- Add GPU memory defragmentation
- Support multi-GPU resource distribution

## Open Questions
- What's the target memory budget per resource category?
- Should we use dedicated allocations for large resources?
- How do we handle device lost scenarios and recovery?
- What's the transient resource pool size strategy?
- Should we implement suballocation for small resources?

## Progress Log
- 2025-12-09: OpenGL provider now materialises transient texture resources on acquire, reusing allocations when descriptors
  match and tagging depth attachments for future framebuffer wiring. Added unit coverage to lock in descriptor tracking
  behaviour.
- 2025-12-08: Added an OpenGL-focused provider that issues native queue/command buffer handles backed by reusable command
  buffers and stub fence/timeline tracking to unblock command encoder integration.

