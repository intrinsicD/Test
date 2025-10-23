# T-0122: Rendering Visibility and Culling System

## Goal
Implement visibility determination and culling systems to optimize rendering by eliminating geometry that won't contribute to the final image.

## Background
- Roadmap alignment: Performance optimization for `RT-002` rendering pipeline.
- Current state: `visibility/` directory is empty. Forward pipeline renders all geometry without culling, causing significant GPU/CPU waste.
- Dependencies: Scene spatial organization, camera frustum data, rendering passes.

## Inputs
- Code: `engine/rendering/include/engine/rendering/`, `engine/scene/`
- Scene data: Entity transforms, bounding volumes, spatial hierarchies
- Camera: View-projection matrices, frustum planes

## Constraints
- CPU culling overhead must be less than GPU savings
- Support dynamic scenes (moving objects)
- Thread-safe for parallel culling
- Integrate seamlessly with existing render passes
- Provide debug visualization options

## Deliverables

### 1. Bounding Volume Hierarchy
**Files**: `engine/rendering/include/engine/rendering/visibility/bvh.hpp` + `.cpp`

#### BoundingVolume
- AABB (axis-aligned bounding box)
- OBB (oriented bounding box)
- Bounding sphere
- Capsule
- Conversion utilities

#### BVHNode
- Tree structure for spatial partitioning
- Support dynamic updates
- SAH (surface area heuristic) construction
- SIMD-optimized traversal

### 2. Frustum Culling
**Files**: `engine/rendering/include/engine/rendering/visibility/frustum_culling.hpp` + `.cpp`

#### Frustum
- Extract from view-projection matrix
- Six plane representation (left, right, top, bottom, near, far)
- Intersection tests with bounding volumes
- SIMD-optimized plane-vs-AABB tests

#### FrustumCuller
- Cull entities against camera frustum
- Multi-threaded culling for large scenes
- Output visibility bitset or filtered entity list
- Support multiple frustums (shadow cascades, mirrors)

### 3. Occlusion Culling
**Files**: `engine/rendering/include/engine/rendering/visibility/occlusion_culling.hpp` + `.cpp`

#### HierarchicalZBuffer
- Build from depth buffer mipmap chain
- Query occlusion for bounding volumes
- GPU-based queries with readback

#### OcclusionQueryManager
- Manage GPU occlusion query objects
- Temporal coherence (use previous frame results)
- Avoid GPU pipeline stalls with double-buffering

#### SoftwareOccluder
- CPU-based occlusion using rasterized depth buffer
- Fast conservative tests
- Useful for early rejection before GPU submission

### 4. Portal and Cell Visibility
**Files**: `engine/rendering/include/engine/rendering/visibility/portal_system.hpp` + `.cpp`

#### Portal
- Convex portal polygon between cells
- Clipping and visibility computation

#### Cell
- Room/region containing geometry
- Connected cells via portals
- Potentially visible set (PVS) computation

#### PortalVisibilitySystem
- Recursive portal traversal
- Determine visible cells from camera
- Integration with scene graph

### 5. Level-of-Detail (LOD) Selection
**Files**: `engine/rendering/include/engine/rendering/visibility/lod_system.hpp` + `.cpp`

#### LODGroup
- Multiple detail levels for same object
- Distance-based or screen coverage-based selection
- Hysteresis to prevent popping

#### LODManager
- Global LOD bias control
- Per-camera LOD selection
- Update visibility list with appropriate LOD level

### 6. Visibility Manager
**Files**: `engine/rendering/include/engine/rendering/visibility/visibility_manager.hpp` + `.cpp`

#### VisibilityManager
- Coordinate all culling systems
- Multi-pass visibility (main view, shadows, reflections)
- Cache and reuse results across passes
- Provide filtered geometry lists to render passes
- Performance tracking and statistics

#### VisibilitySet
- Compact representation of visible entities
- Bitset or sparse list
- Query interface for render passes

### 7. Compute Shader Culling
**Files**: `engine/rendering/include/engine/rendering/visibility/gpu_culling.hpp` + `.cpp`

#### GPUCullingPass
- GPU-driven frustum culling compute shader
- Write visible instance indices to buffer
- Support indirect draw generation
- Hi-Z occlusion testing on GPU

### 8. Debug Visualization
**Files**: `engine/rendering/include/engine/rendering/visibility/debug_draw.hpp` + `.cpp`

- Visualize frustum planes
- Draw bounding volumes (color-coded by visibility)
- Visualize BVH structure
- Display occlusion buffer
- Show portal connections
- Culling statistics overlay

### 9. Integration with Render Passes
- Modify `ForwardGeometryPass` to accept visibility set
- Provide visibility query API for custom passes
- Support per-pass culling requirements (e.g., shadow casters only)

### 10. Testing
- Unit tests for bounding volume math
- Test frustum extraction and culling
- Verify occlusion query correctness
- Performance benchmarks with various scene sizes
- Regression tests for edge cases (objects at frustum boundaries)

### 11. Documentation
- Document visibility architecture in `docs/modules/rendering/visibility-culling.md`
- Culling strategies and trade-offs
- Performance tuning guide
- Integration examples

## Work Breakdown
1. Implement bounding volume library (priority: critical)
2. Implement frustum culling (priority: critical)
3. Integrate with forward pipeline (priority: critical)
4. Implement BVH construction and queries (priority: high)
5. Implement LOD system (priority: high)
6. Implement occlusion culling (priority: medium)
7. Implement GPU culling (priority: medium)
8. Implement portal system (priority: low)
9. Add debug visualization (priority: medium)
10. Add comprehensive testing and benchmarking
11. Document architecture and usage

## Acceptance Criteria
- [ ] Frustum culling eliminates off-screen geometry
- [ ] BVH accelerates culling for scenes with >10k objects
- [ ] Occlusion culling reduces overdraw by >30% in complex scenes
- [ ] LOD system switches detail levels based on distance
- [ ] GPU culling produces identical results to CPU culling
- [ ] Culling overhead <2ms for 100k object scene
- [ ] Integration with render passes shows measurable performance gain
- [ ] Debug visualization aids in development
- [ ] Tests verify correctness and performance
- [ ] Documentation covers all culling techniques

## Metrics & Benchmarks
- Frustum culling: Reduce rendered objects by 60-80% in typical scenes
- Occlusion culling: Additional 20-40% reduction in dense environments
- BVH traversal: <100ns per entity for spatial queries
- LOD system: Smooth transitions without visible popping
- Overall frame time reduction: 30-50% in complex scenes
- CPU culling overhead: <10% of saved GPU time

## Follow-Up
- Implement software occlusion culling with SSE/AVX
- Add support for temporal coherence in culling
- Implement conservative rasterization for better occlusion
- Add cluster-based culling for instanced geometry
- Integrate with streaming system for progressive loading

## Open Questions
- Should we support frustum expansion for shadow stabilization?
- What's the optimal BVH rebuild frequency for dynamic scenes?
- Should occlusion queries be mandatory or optional feature?
- How do we handle transparent objects in occlusion culling?
- What's the strategy for culling skeletal meshes (use mesh AABB or bone AABBs)?

