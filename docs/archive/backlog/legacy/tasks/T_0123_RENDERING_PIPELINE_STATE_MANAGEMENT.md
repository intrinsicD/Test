# T-0123: Rendering Pipeline State Management

## Goal
Implement graphics pipeline state object (PSO) management for efficient state tracking, caching, and binding across different graphics APIs.

## Background
- Roadmap alignment: Core infrastructure for `RT-003` backend implementation.
- Current state: `pipeline/` directory is empty. No centralized pipeline state management exists.
- Dependencies: Command encoder (T-0119), resource provider (T-0120).

## Inputs
- Code: Backend schedulers, command encoders
- Graphics API docs: Vulkan pipeline objects, D3D12 PSOs, Metal render pipeline state
- Material system: Shader binding, blend modes, depth/stencil settings

## Constraints
- Support all modern graphics APIs (Vulkan, D3D12, Metal, OpenGL)
- Minimize state changes through intelligent sorting and caching
- Thread-safe pipeline creation (parallel shader compilation)
- Fast hash-based lookups
- Graceful degradation if state creation fails

## Deliverables

### 1. Pipeline State Description
**Files**: `engine/rendering/include/engine/rendering/pipeline/pipeline_state.hpp` + `.cpp`

#### GraphicsPipelineState
- Shader stages (vertex, fragment, geometry, tessellation)
- Vertex input layout (attributes, bindings, strides)
- Input assembly (topology, primitive restart)
- Rasterization state (cull mode, fill mode, depth bias)
- Depth/stencil state (depth test, depth write, stencil ops)
- Blend state (per-render-target blend modes, blend factors)
- Multisampling state (sample count, alpha-to-coverage)
- Dynamic state flags (viewport, scissor, blend constants)
- Render pass compatibility
- Hash function for cache lookups

#### ComputePipelineState
- Compute shader
- Push constants / specialization constants
- Thread group size
- Hash function

### 2. Shader Management
**Files**: `engine/rendering/include/engine/rendering/pipeline/shader.hpp` + `.cpp`

#### ShaderStage
- Shader type enumeration (vertex, fragment, compute, etc.)
- Bytecode storage (SPIR-V, DXIL, Metal bytecode, GLSL source)
- Entry point name
- Reflection data (inputs, outputs, uniforms, samplers)

#### ShaderModule
- Compiled shader representation
- Backend-specific handles (VkShaderModule, ID3D12ShaderBytecode, etc.)
- Compilation from source or bytecode
- Hot-reload support

#### ShaderLibrary
- Load and cache shaders by handle
- Compilation queue for async loading
- Error reporting and fallback shaders

### 3. Vertex Input Layout
**Files**: `engine/rendering/include/engine/rendering/pipeline/vertex_layout.hpp` + `.cpp`

#### VertexAttribute
- Semantic (position, normal, UV, color, etc.)
- Format (float3, float2, uint32, etc.)
- Offset within vertex
- Input slot/binding

#### VertexInputLayout
- List of attributes
- Per-binding stride and input rate (per-vertex/per-instance)
- Compatibility checking
- Common presets (P, PN, PNT, PNTC, skinned, etc.)

### 4. Pipeline Cache
**Files**: `engine/rendering/include/engine/rendering/pipeline/pipeline_cache.hpp` + `.cpp`

#### PipelineCache
- Hash map of pipeline states to backend objects
- LRU eviction for memory management
- Serialize/deserialize cache to disk
- Warmup from saved cache on startup
- Thread-safe concurrent access
- Statistics (hit rate, creation time)

#### PipelineCacheManager
- Per-backend cache instances
- Global cache coordination
- Cache file management

### 5. Backend Pipeline Objects

#### Vulkan Pipeline
- **File**: `engine/rendering/src/backend/vulkan/pipeline.cpp` + `.hpp`
- Create `VkPipeline` from `GraphicsPipelineState`
- Pipeline layout creation
- Descriptor set layout management
- Pipeline cache integration

#### DirectX12 Pipeline
- **File**: `engine/rendering/src/backend/directx12/pipeline.cpp` + `.hpp`
- Create `ID3D12PipelineState` from description
- Root signature creation and caching
- PSO library integration

#### Metal Pipeline
- **File**: `engine/rendering/src/backend/metal/pipeline.cpp` + `.hpp`
- Create `MTLRenderPipelineState`
- Depth/stencil state objects
- Pipeline state descriptor setup

#### OpenGL Pipeline
- **File**: `engine/rendering/src/backend/opengl/pipeline.cpp` + `.hpp`
- State tracking and restoration
- Program pipeline objects (ARB_separate_shader_objects)
- Vertex array object binding

### 6. Render State Management
**Files**: `engine/rendering/include/engine/rendering/pipeline/render_state.hpp` + `.cpp`

#### RenderState
- Current bound pipeline
- Current bound vertex/index buffers
- Current descriptor sets / resource bindings
- Current viewport/scissor
- Dirty state tracking

#### StateTracker
- Minimize redundant state changes
- Compare and set pattern
- Backend-specific optimizations

### 7. Material Pipeline Integration
**Files**: Integration with existing material system

- Material describes pipeline state requirements
- Material-to-PSO mapping
- Support pipeline variants (different vertex layouts, blend modes)

### 8. Pipeline Compilation
**Files**: `engine/rendering/include/engine/rendering/pipeline/compiler.hpp` + `.cpp`

#### PipelineCompiler
- Async pipeline creation
- Job queue for parallel compilation
- Progress tracking
- Error handling and reporting
- Fallback to default pipeline on error

### 9. Testing
- Unit tests for pipeline state hashing
- Test cache hit/miss scenarios
- Verify backend pipeline creation
- Stress test with many pipeline variants
- Test shader compilation and reflection
- Benchmark cache performance

### 10. Documentation
- Document pipeline architecture in `docs/modules/rendering/pipeline-state.md`
- Backend-specific pipeline details
- Pipeline variant strategies
- Shader compilation pipeline
- Performance tuning guide

## Work Breakdown
1. Implement pipeline state descriptors (priority: critical)
2. Implement shader management (priority: critical)
3. Implement vertex layout system (priority: critical)
4. Implement pipeline cache (priority: high)
5. Implement Vulkan pipeline backend (priority: high)
6. Implement OpenGL pipeline backend (priority: medium)
7. Implement DirectX12 pipeline backend (priority: medium)
8. Implement Metal pipeline backend (priority: low)
9. Integrate with material system
10. Add async compilation
11. Add comprehensive testing
12. Document architecture

## Acceptance Criteria
- [ ] `GraphicsPipelineState` correctly describes all render state
- [ ] `PipelineCache` provides fast lookups and reduces redundant creation
- [ ] Vulkan backend creates valid `VkPipeline` objects
- [ ] Shader library loads and compiles shaders correctly
- [ ] Vertex input layout supports common formats
- [ ] Pipeline cache serializes/deserializes correctly
- [ ] All backends implement pipeline creation
- [ ] Material system integrates with pipeline state
- [ ] Async compilation improves load times
- [ ] Tests verify correctness and performance
- [ ] Documentation covers pipeline lifecycle

## Metrics & Benchmarks
- Pipeline cache hit rate >95% after warmup
- Pipeline creation time: <10ms per pipeline (cached) or <100ms (compiled)
- Hash collision rate <0.01%
- State change overhead: <50ns per state check
- Cache serialization: <100ms for 1000 pipelines
- Parallel compilation: 4x speedup on 8-core CPU

## Follow-Up
- Implement shader variant system (ubershaders with specialization)
- Add runtime pipeline optimization/recompilation
- Implement shader hot-reload for development
- Add pipeline statistics and profiling
- Support ray tracing pipelines (DXR/Vulkan RT)

## Open Questions
- Should we support runtime pipeline switching (e.g., quality settings)?
- What's the optimal cache eviction policy?
- Should we precompile common pipelines at startup?
- How do we handle pipeline compatibility across GPU generations?
- What's the strategy for shader permutations (avoid combinatorial explosion)?

