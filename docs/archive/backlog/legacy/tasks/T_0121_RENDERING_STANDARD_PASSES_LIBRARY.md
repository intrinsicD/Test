# T-0121: Standard Rendering Passes Library

## Goal
Implement a comprehensive library of standard render passes to enable complete rendering pipelines beyond the minimal forward geometry pass.

## Background
- Roadmap alignment: Essential for `RT-002` (rendering pipeline MVP) completion.
- Current state: Only `ForwardGeometryPass` exists (inline in `forward_pipeline.cpp`). The `passes/` directory is empty.
- Dependencies: Command encoder implementation (T-0119), resource provider (T-0120).

## Inputs
- Code: `engine/rendering/include/engine/rendering/render_pass.hpp`, `engine/rendering/src/forward_pipeline.cpp`
- Reference: Existing `ForwardGeometryPass` as template
- Research: Modern rendering techniques (PBR, shadow mapping, deferred shading, post-processing)

## Constraints
- All passes must use the frame-graph API correctly
- Backend-agnostic implementation using `CommandEncoder` interface
- Configurable parameters via pass constructors
- Support incremental adoption (passes work independently)
- Follow engine coding style and naming conventions

## Deliverables

### 1. Shadow Mapping Passes
**Files**: `engine/rendering/include/engine/rendering/passes/shadow_pass.hpp` + `.cpp`

#### DirectionalShadowPass
- Render scene from light's perspective to shadow map texture
- Configurable shadow map resolution
- Support cascaded shadow maps (CSM) for multiple frustum splits
- Output: depth texture array

#### PointLightShadowPass
- Render cubemap shadow map for point lights
- Six render passes (one per cube face) or single-pass with geometry shader
- Output: depth cubemap texture

#### SpotLightShadowPass
- Render cone-shaped shadow map
- Output: single depth texture

### 2. Lighting Passes
**Files**: `engine/rendering/include/engine/rendering/passes/lighting_pass.hpp` + `.cpp`

#### DeferredLightingPass
- Read G-buffer (albedo, normal, depth, roughness/metallic)
- Compute lighting for all lights in scene
- Output: HDR color texture
- Support directional, point, and spot lights

#### ForwardLightingPass
- Enhanced forward pass with multiple light support
- Compute lighting per-fragment during geometry pass
- Support shadow map sampling
- Output: HDR color texture

#### AmbientOcclusionPass
- Screen-space ambient occlusion (SSAO) or horizon-based (HBAO)
- Input: depth/normal textures
- Output: AO texture

### 3. G-Buffer Generation
**Files**: `engine/rendering/include/engine/rendering/passes/gbuffer_pass.hpp` + `.cpp`

#### GBufferPass
- Render scene geometry to multiple render targets
- Output MRTs: albedo (RGB), normal (RGB), depth, material properties (roughness/metallic/AO)
- Optimized for deferred rendering

### 4. Post-Processing Passes
**Files**: `engine/rendering/include/engine/rendering/passes/postprocess/` directory

#### ToneMappingPass
- HDR to LDR conversion
- Support multiple operators: ACES, Reinhard, Uncharted 2
- Exposure control
- Input: HDR color, Output: LDR color

#### BloomPass
- Multi-pass: downsample, blur, upsample, composite
- Configurable threshold and intensity
- Input: HDR color, Output: bloom texture

#### TemporalAntiAliasingPass (TAA)
- Jitter projection matrix
- Sample history buffer with motion vectors
- Resolve with configurable feedback weight
- Input: current frame, history, motion vectors
- Output: anti-aliased color

#### FXAAPass
- Fast approximate anti-aliasing
- Single-pass shader-based AA
- Input: LDR color, Output: anti-aliased color

#### ColorGradingPass
- LUT-based color grading
- Support custom 3D LUTs
- Input: color texture, Output: graded color

### 5. Utility Passes
**Files**: `engine/rendering/include/engine/rendering/passes/utility/` directory

#### ClearPass
- Clear render targets to specified values
- Support color, depth, stencil clears
- Configurable clear values

#### CopyPass
- Copy texture to texture or buffer to buffer
- Support format conversion if compatible
- Useful for mipmap generation, resolve, etc.

#### BlitPass
- Blit with optional filtering and region selection
- Support different source/destination sizes

#### MipmapGenerationPass
- Generate mipmaps for texture resources
- Compute-shader based or graphics-based approaches

#### PresentationPass
- Final blit to swapchain
- Handle backbuffer format conversion
- Output to window surface

### 6. Depth and Stencil Passes
**Files**: `engine/rendering/include/engine/rendering/passes/depth_pass.hpp` + `.cpp`

#### DepthPrePass
- Early-Z optimization
- Render depth only, no color
- Output: depth texture for main pass reuse

#### HierarchicalZPass
- Build hi-Z pyramid for occlusion culling
- Downsample depth buffer in compute shader
- Output: mipmap chain of depth

### 7. Debug Visualization Passes
**Files**: `engine/rendering/include/engine/rendering/passes/debug/` directory

#### WireframePass
- Render geometry as wireframe
- Overlay on top of shaded geometry

#### NormalVisualizationPass
- Render normals as RGB colors
- Useful for debugging geometry

#### DepthVisualizationPass
- Visualize depth buffer as grayscale
- Configurable near/far plane mapping

### 8. Testing
- Unit tests for each pass type
- Verify frame-graph resource declarations
- Test pass execution with mock encoder
- Integration tests with full rendering pipeline
- Visual regression tests (compare screenshots)

### 9. Documentation
- Document each pass in `docs/modules/rendering/passes/`
- Usage examples and configuration
- Performance characteristics
- Integration with frame-graph

## Work Breakdown
1. Implement shadow mapping passes (priority: high)
2. Implement lighting passes (priority: high)
3. Implement G-buffer pass (priority: high)
4. Implement post-processing passes (priority: medium)
5. Implement utility passes (priority: medium)
6. Implement depth passes (priority: medium)
7. Implement debug passes (priority: low)
8. Add comprehensive testing
9. Document all passes

## Acceptance Criteria
- [ ] Shadow passes generate valid shadow maps for directional/point/spot lights
- [ ] Lighting passes compute physically-based lighting with shadow sampling
- [ ] G-buffer pass outputs complete material data for deferred rendering
- [ ] Post-processing passes produce expected visual results (tonemapping, bloom, AA)
- [ ] Utility passes correctly clear/copy/blit resources
- [ ] Depth passes optimize rendering performance
- [ ] Debug passes aid in development and troubleshooting
- [ ] All passes integrate with frame-graph correctly
- [ ] Tests verify pass behavior and resource usage
- [ ] Documentation covers all passes with examples

## Metrics & Benchmarks
- Shadow pass overhead: <2ms for 2048x2048 shadow map
- Lighting pass: <5ms for scene with 100 dynamic lights
- Post-processing total: <3ms for 1080p frame
- G-buffer pass comparable to forward geometry pass
- All passes stay within frame budget (16.67ms for 60fps)

## Follow-Up
- Implement volumetric lighting/fog
- Add screen-space reflections (SSR)
- Implement ray-traced shadows/reflections/AO
- Add temporal upsampling (DLSS-style)
- Implement advanced post-processing (motion blur, DOF, lens flares)

## Open Questions
- Should we provide pass presets/templates for common pipelines?
- How do we handle pass parameter animation/keyframing?
- Should passes expose telemetry/profiling hooks?
- What's the strategy for pass variants (quality levels)?

