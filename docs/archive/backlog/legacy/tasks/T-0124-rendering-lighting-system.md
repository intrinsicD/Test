# T-0124: Rendering Lighting System

## Goal
Implement a comprehensive lighting system supporting multiple light types, shadow mapping, and physically-based shading models.

## Background
- Roadmap alignment: Essential for `RT-002` rendering pipeline completeness.
- Current state: `lighting/` directory is empty. No light data structures, culling, or GPU data management.
- Dependencies: Standard passes library (T-0121), visibility system (T-0122), resource provider (T-0120).

## Inputs
- Code: Scene ECS system, rendering components
- Research: PBR theory, shadow mapping techniques, light culling algorithms
- Reference: Modern game engines (UE, Unity, Frostbite)

## Constraints
- Support hundreds to thousands of dynamic lights
- Efficient light culling for large scenes
- GPU-friendly data layouts
- Support both forward and deferred rendering
- Physically-based shading models
- Extensible for custom light types

## Deliverables

### 1. Light Types and Components
**Files**: `engine/rendering/include/engine/rendering/lighting/light_components.hpp` + `.cpp`

#### DirectionalLight Component
- Direction vector
- Color and intensity
- Cascaded shadow map settings (split distances, shadow map resolution)
- Shadow bias parameters

#### PointLight Component
- Position (from transform)
- Color and intensity
- Attenuation radius
- Shadow cube map settings
- Shadow bias

#### SpotLight Component
- Position and direction
- Color and intensity
- Inner/outer cone angles
- Attenuation radius
- Shadow map settings

#### AreaLight Component (future)
- Rectangular/spherical area lights
- Two-sided emission
- For more realistic lighting

### 2. Light Data Management
**Files**: `engine/rendering/include/engine/rendering/lighting/light_manager.hpp` + `.cpp`

#### LightManager
- Gather lights from scene ECS
- Convert to GPU-friendly structures
- Allocate and upload to GPU buffers
- Support structured buffers / SSBOs
- Per-frame light list updates

#### GPULightData
- Packed light structures for shaders
- Common format across light types (tagged union or separate arrays)
- SIMD-friendly alignment
- Shadow map indices

### 3. Light Culling
**Files**: `engine/rendering/include/engine/rendering/lighting/light_culling.hpp` + `.cpp`

#### TiledLightCulling
- Divide screen into tiles (16x16 or 32x32)
- Frustum culling per tile
- Output: per-tile light index lists
- CPU or GPU compute implementation

#### ClusteredLightCulling
- 3D grid in view space
- Assign lights to clusters based on AABB overlap
- Output: per-cluster light index lists
- Better depth precision than tiled

#### LightCullingManager
- Choose culling strategy based on scene
- Manage light index buffers
- Provide culling results to shaders

### 4. Shadow Mapping System
**Files**: `engine/rendering/include/engine/rendering/lighting/shadow_system.hpp` + `.cpp`

#### ShadowMapAtlas
- Pack multiple shadow maps into texture atlas
- Dynamic allocation of atlas regions
- Configurable atlas size
- Eviction policy for inactive shadows

#### CascadedShadowMapManager
- Compute cascade split distances (logarithmic, PSSM)
- Generate view-proj matrices for each cascade
- Stabilization (texel-aligned, snap to texel grid)
- Blend between cascades

#### ShadowCaster
- Mark entities as shadow casters
- Separate culling for shadow passes
- LOD selection for shadows

#### ShadowMapCache
- Cache shadow maps for static lights
- Invalidation on object movement
- Reduce shadow map renders per frame

### 5. PBR Shading Models
**Files**: `engine/rendering/include/engine/rendering/lighting/shading_models.hpp` + GLSL/HLSL shaders

#### Material Properties
- Albedo/base color
- Metallic
- Roughness
- Normal map
- Ambient occlusion
- Emissive

#### BRDF Functions (shader code)
- Cook-Torrance microfacet model
- Diffuse: Lambert or Disney diffuse
- Specular: GGX/Trowbridge-Reitz
- Fresnel: Schlick approximation
- Visibility/geometry term

#### Image-Based Lighting (IBL)
- Environment map support
- Diffuse irradiance cubemap
- Specular filtered cubemap (prefiltered for roughness)
- BRDF integration LUT
- Split-sum approximation

### 6. Shadow Filtering
**Files**: Shader implementations in `engine/rendering/shaders/lighting/`

#### PCF (Percentage Closer Filtering)
- Multiple taps around shadow texel
- Configurable kernel size (3x3, 5x5, 7x7)

#### VSM (Variance Shadow Mapping)
- Store depth and depth² in shadow map
- Chebyshev inequality for soft shadows
- Handle light bleeding

#### ESM (Exponential Shadow Mapping)
- Exponential warp
- Gaussian blur for soft shadows

#### PCSS (Percentage Closer Soft Shadows)
- Contact-hardening shadows
- Blocker search + adaptive PCF

### 7. Light Probes and GI
**Files**: `engine/rendering/include/engine/rendering/lighting/light_probes.hpp` + `.cpp`

#### LightProbe Component
- Position in world space
- Baked irradiance/radiance data
- Spherical harmonics coefficients

#### LightProbeGrid
- Regular 3D grid of probes
- Interpolation between probes for any point
- Update system for dynamic GI

#### ReflectionProbe Component
- Cubemap capture for local reflections
- Parallax-corrected cubemap projection
- Influence volume (box or sphere)

### 8. Integration with Render Passes
- Update light passes (T-0121) to use LightManager data
- Pass light buffers to shaders
- Bind shadow maps as shader resources
- Support both forward and deferred lighting

### 9. Debug Visualization
**Files**: `engine/rendering/include/engine/rendering/lighting/debug_lighting.hpp` + `.cpp`

- Visualize light volumes (spheres for point lights, cones for spots)
- Show light influence on grid
- Display shadow map contents
- Cascade visualization for CSM
- Light heatmaps (overlapping lights)
- PBR material property visualization

### 10. Testing
- Unit tests for light culling algorithms
- Verify shadow matrix computation
- Test light data packing/unpacking
- Validate PBR shading equations
- Performance benchmarks for various light counts
- Visual tests for shadow quality

### 11. Documentation
- Document lighting architecture in `docs/modules/rendering/lighting-system.md`
- PBR material guide
- Shadow mapping best practices
- Light culling strategies
- Performance optimization tips

## Work Breakdown
1. Implement light components and manager (priority: critical)
2. Implement basic PBR shading (priority: critical)
3. Implement directional light with CSM (priority: high)
4. Implement point and spot lights (priority: high)
5. Implement basic shadow mapping (priority: high)
6. Implement light culling (tiled then clustered) (priority: medium)
7. Implement advanced shadow filtering (priority: medium)
8. Implement IBL and light probes (priority: medium)
9. Add debug visualization (priority: low)
10. Add comprehensive testing
11. Document architecture and usage

## Acceptance Criteria
- [ ] LightManager collects and uploads light data to GPU
- [ ] Directional lights with cascaded shadow maps render correctly
- [ ] Point lights with cube shadow maps work
- [ ] Spot lights with projection shadow maps work
- [ ] PBR shading produces physically plausible results
- [ ] Light culling reduces shading cost in scenes with many lights
- [ ] Shadow filtering produces smooth shadow edges
- [ ] IBL provides realistic ambient lighting
- [ ] Debug tools aid in lighting setup
- [ ] Tests verify correctness and performance
- [ ] Documentation covers lighting pipeline

## Metrics & Benchmarks
- Support 1000+ point lights with clustered culling
- CSM overhead: <3ms for 4 cascades at 2048x2048
- Light culling: <1ms for 10k lights on screen
- PBR shading: <0.5ms per light per million pixels
- Shadow map rendering: <1ms per directional light
- Light buffer upload: <0.5ms per frame

## Follow-Up
- Implement volumetric lighting/fog
- Add light scattering effects
- Implement contact shadows
- Add ray-traced shadows/GI
- Implement dynamic global illumination (DDGI, SVOGI)
- Add light animation/IES profiles

## Open Questions
- Should we support more than 4 shadow cascades?
- What's the default shadow map resolution?
- Should lights have falloff curves or just inverse-square?
- How do we handle translucent shadow receivers?
- What's the strategy for very large scenes (many directional lights)?
# T-0119: Rendering Command Encoder Implementation

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

