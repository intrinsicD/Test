# RG-450 Node Descriptor API Design

**Status:** Draft  
**Task:** RG-450-A  
**Owner:** rendering-lead  
**Date:** 2025-11-06  

## Executive Summary

This document specifies the node descriptor API for the modular render pipeline planner, enabling plugin-provided render passes to self-describe their resource contracts, queue preferences, and execution dependencies. The design builds on the existing `INode` interface in `engine/rendering/include/engine/rendering/frame_graph_node.hpp` and aligns with ADR-0003's frame-graph determinism requirements.

## Goals

1. **Declarative reflection:** Nodes advertise created/read/written resources without procedural callbacks
2. **Hot-reload safety:** Descriptor changes trigger deterministic plan rebuilds without state corruption
3. **Queue awareness:** Nodes express queue preferences (graphics/compute/transfer) for async execution
4. **Validation support:** Descriptors enable compile-time detection of resource conflicts and missing dependencies
5. **Plugin integration:** Works with `SubsystemRegistry` and `ISubsystemInterface` for hot-swappable node factories

## Non-Goals

- Runtime modification of node descriptors (changes require plan rebuild)
- Dynamic resource sizing based on viewport (descriptors are static per node type)
- Cross-graph resource sharing (Phase 1 focuses on single frame-graph execution)

## Background

### Current State

From existing implementation (`engine/rendering/include/engine/rendering/frame_graph_node.hpp`):

```cpp
struct NodeDescriptor {
    std::string id;
    std::vector<ResourceDesc> creates;
    std::vector<ResourceUse> reads;
    std::vector<ResourceUse> writes;
    std::vector<std::string> tags;
    QueueType preferred_queue{QueueType::Graphics};
};

class INode {
    virtual const NodeDescriptor& Reflect() const = 0;
    virtual void Compile(NodeContext& context) = 0;
    virtual void Execute(NodeContext& context) = 0;
};
```

**Implemented:**
- ✅ Basic `NodeDescriptor` with resource declarations
- ✅ `INode` interface with reflection, compile, execute phases
- ✅ `ResourceDesc` for created resources (textures, buffers, TLAS)
- ✅ `ResourceUse` for read/write access patterns
- ✅ Helper methods: `declares_resource()`, `find_created()`, `find_read()`, `find_write()`

**Gap:** Missing comprehensive design documentation for:
- Node descriptor validation rules
- Resource lifecycle semantics (transient vs persistent)
- Tag-based filtering for pipeline profiles
- Example implementations for common pass types
- Integration with `FrameGraphRegistry` and plugin hot-reload

### References

- [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](../specs/ADR_0003_RUNTIME_FRAME_GRAPH.md)
- [`engine/rendering/include/engine/rendering/frame_graph_node.hpp`](../../engine/rendering/include/engine/rendering/frame_graph_node.hpp)
- [`engine/rendering/include/engine/rendering/frame_graph_types.hpp`](../../engine/rendering/include/engine/rendering/frame_graph_types.hpp)
- [`docs/design/PLUGIN_ARCHITECTURE.md`](PLUGIN_ARCHITECTURE.md)

## API Specification

### 1. Resource Declaration

#### ResourceDesc — Created Resources

Describes resources produced by a node:

```cpp
struct ResourceDesc {
    std::string name;                    // Unique identifier within frame graph
    ResourceKind kind;                   // Texture, Buffer, TLAS, etc.
    ResourceFormat format;               // Pixel/data format
    ResourceDimension dimension;         // 1D, 2D, 3D, Cube
    uint32_t width, height, depth;       // Extents
    uint32_t array_layers;               // Array/cube layer count
    uint32_t mip_levels;                 // Mipmap chain length
    ResourceSampleCount sample_count;    // MSAA samples
    bool transient;                      // Transient (aliased) vs persistent
};
```

**Validation Rules:**
- `name` must be unique across all nodes in the frame graph
- `width`, `height`, `depth` must be > 0
- `array_layers` and `mip_levels` must be ≥ 1
- `transient = true` enables automatic aliasing; `false` preserves across frames
- `kind = External` indicates runtime-provided resource (swapchain, XR views)

#### ResourceUse — Consumed Resources

Describes how a node accesses existing resources:

```cpp
struct ResourceUse {
    std::string name;                    // Resource identifier
    PipelineStage stage;                 // Execution stage (vertex, fragment, compute)
    Access access;                       // Read, Write, ReadWrite
    ResourceState state;                 // Expected layout/state

    bool is_read_only() const;
    bool is_write() const;
};
```

**Validation Rules:**
- `name` must reference a resource created by another node or marked as external
- Write access requires exclusive ownership (no other nodes writing simultaneously)
- Read-after-write requires explicit dependency declaration
- `state` must be compatible with `access` and `stage`

### 2. Node Descriptor Structure

Complete descriptor with validation semantics:

```cpp
struct NodeDescriptor {
    std::string id;                      // Unique node identifier
    std::vector<ResourceDesc> creates;   // Resources produced
    std::vector<ResourceUse> reads;      // Resources consumed (read-only)
    std::vector<ResourceUse> writes;     // Resources consumed (write)
    std::vector<std::string> tags;       // Profile/feature tags
    QueueType preferred_queue;           // Graphics, Compute, Transfer

    // Validation helpers
    bool declares_resource(std::string_view name) const;
    const ResourceDesc* find_created(std::string_view name) const;
    const ResourceUse* find_read(std::string_view name) const;
    const ResourceUse* find_write(std::string_view name) const;
};
```

**Semantic Guarantees:**

1. **Uniqueness:** `id` must be unique within the frame graph registry
2. **Exclusivity:** Resource cannot appear in both `reads` and `writes` for same node
3. **Provenance:** All read/written resources must be created by current or prior node
4. **Queue compatibility:** `preferred_queue` must support declared pipeline stages

### 3. INode Interface Semantics

```cpp
class INode {
public:
    virtual ~INode() = default;

    // Phase 1: Reflection — Called once during plan construction
    virtual const NodeDescriptor& Reflect() const = 0;

    // Phase 2: Compilation — Called when graph plan is built
    virtual void Compile(NodeContext& context) = 0;

    // Phase 3: Execution — Called per frame during graph execution
    virtual void Execute(NodeContext& context) = 0;
};
```

#### Reflection Phase

**Called:** Once during node registration or after hot-reload  
**Purpose:** Provide static descriptor for planner validation and scheduling  
**Contract:**
- Must return stable reference (same address across calls)
- Descriptor must not change without triggering rebuild
- Should not allocate or access GPU resources

**Example:**
```cpp
class GBufferNode : public INode {
    const NodeDescriptor& Reflect() const override {
        static const NodeDescriptor desc = {
            .id = "gbuffer",
            .creates = {
                {.name = "gbuffer_albedo", .kind = ResourceKind::Texture,
                 .format = ResourceFormat::Rgba8Unorm, .width = 1920, .height = 1080},
                {.name = "gbuffer_normal", .kind = ResourceKind::Texture,
                 .format = ResourceFormat::Rgba16f, .width = 1920, .height = 1080},
                {.name = "gbuffer_depth", .kind = ResourceKind::Texture,
                 .format = ResourceFormat::Depth32f, .width = 1920, .height = 1080}
            },
            .reads = {},
            .writes = {},
            .tags = {"pbr", "deferred"},
            .preferred_queue = QueueType::Graphics
        };
        return desc;
    }
};
```

#### Compilation Phase

**Called:** When frame graph plan is built/rebuilt  
**Purpose:** Prepare GPU resources, compile shaders, create pipeline states  
**Contract:**
- Can allocate persistent GPU resources
- Can query resource metadata via `context.resource_info()`
- Must not submit GPU commands
- Should be deterministic (same inputs → same outputs)

**Example:**
```cpp
void GBufferNode::Compile(NodeContext& context) {
    // Query actual allocated resource dimensions
    auto albedo_info = context.resource_info("gbuffer_albedo");
    auto normal_info = context.resource_info("gbuffer_normal");
    auto depth_info = context.resource_info("gbuffer_depth");

    // Compile shaders with resource bindings
    pipeline_ = compile_gbuffer_pipeline(
        albedo_info.handle, normal_info.handle, depth_info.handle);

    // Cache render pass configuration
    render_pass_ = create_render_pass({albedo_info, normal_info, depth_info});
}
```

#### Execution Phase

**Called:** Every frame during graph execution  
**Purpose:** Record GPU commands to render target  
**Contract:**
- Can record commands via `context.command_encoder()`
- Can read resource handles via `context.resource_handle()`
- Must execute deterministically for telemetry/replay
- Should respect budgets for performance monitoring

**Example:**
```cpp
void GBufferNode::Execute(NodeContext& context) {
    auto& encoder = context.command_encoder();
    
    // Begin render pass
    encoder.begin_render_pass(render_pass_);
    
    // Bind pipeline and draw geometry
    encoder.bind_pipeline(pipeline_);
    encoder.draw_indexed(mesh_->index_count);
    
    // End render pass
    encoder.end_render_pass();
}
```

### 4. Tag-Based Pipeline Profiles

Tags enable conditional node inclusion based on rendering profile:

```cpp
// Common tag conventions
static constexpr std::string_view TAG_PBR = "pbr";
static constexpr std::string_view TAG_DEFERRED = "deferred";
static constexpr std::string_view TAG_FORWARD = "forward";
static constexpr std::string_view TAG_RAY_TRACING = "ray_tracing";
static constexpr std::string_view TAG_COMPUTE = "compute";
static constexpr std::string_view TAG_POST_PROCESS = "post_process";
static constexpr std::string_view TAG_DEBUG = "debug";
static constexpr std::string_view TAG_VR = "vr";
static constexpr std::string_view TAG_HEADLESS = "headless";
```

**Profile Assembly Example:**
```cpp
// Build PBR deferred pipeline
auto pbr_profile = FrameGraphPlanner::BuildProfile({
    .required_tags = {"pbr", "deferred"},
    .excluded_tags = {"debug"},
    .enable_async_compute = true
});

// Build forward + ray tracing hybrid
auto hybrid_profile = FrameGraphPlanner::BuildProfile({
    .required_tags = {"forward", "ray_tracing"},
    .excluded_tags = {},
    .enable_async_compute = true
});
```

### 5. Queue Type Preferences

Nodes express preferred execution queue for async scheduling:

```cpp
enum class QueueType {
    Graphics,    // Supports graphics + compute + transfer
    Compute,     // Compute + transfer only
    Transfer,    // Transfer only
    Any          // Planner decides based on availability
};
```

**Scheduling Rules:**
1. Graphics-only operations (render passes) → `QueueType::Graphics`
2. Pure compute (post-processing, physics) → `QueueType::Compute`
3. Data uploads/downloads → `QueueType::Transfer`
4. Flexible operations (barrier injection) → `QueueType::Any`

**Queue Conflict Resolution:**
- If node requires graphics stages but declares `QueueType::Compute`, planner rejects with diagnostic
- If compute queue unavailable, planner falls back to graphics queue with warning
- Transfer-only operations automatically scheduled on dedicated transfer queue if available

### 6. NodeContext API

Context provided during compilation and execution:

```cpp
class NodeContext {
public:
    // Render execution context (frame number, telemetry, etc.)
    RenderExecutionContext& render_context() const;

    // Command encoder for recording GPU work
    CommandEncoder& command_encoder() const;

    // Queue assigned by planner
    QueueType queue() const;

    // Node's own descriptor
    const NodeDescriptor& descriptor() const;

    // Resource handle lookup by name
    FrameGraphResourceHandle resource_handle(std::string_view name) const;

    // Resource metadata (format, dimensions, etc.)
    const FrameGraphResourceInfo& resource_info(FrameGraphResourceHandle handle) const;
    const FrameGraphResourceInfo& resource_info(std::string_view name) const;
};
```

**Usage Pattern:**
```cpp
void MyNode::Execute(NodeContext& ctx) {
    // Get allocated resource dimensions (may differ from descriptor)
    auto input_tex = ctx.resource_info("input_texture");
    auto output_tex = ctx.resource_info("output_texture");

    // Record commands
    auto& encoder = ctx.command_encoder();
    encoder.dispatch_compute(
        (output_tex.width + 15) / 16,
        (output_tex.height + 15) / 16,
        1);
}
```

## Example Node Implementations

### Example 1: Simple Geometry Pass

```cpp
class GeometryPassNode : public INode {
public:
    const NodeDescriptor& Reflect() const override {
        static const NodeDescriptor desc = {
            .id = "geometry_pass",
            .creates = {
                {.name = "color_output", .kind = ResourceKind::Texture,
                 .format = ResourceFormat::Rgba8Unorm, .width = 1920, .height = 1080,
                 .transient = true}
            },
            .reads = {},
            .writes = {},
            .tags = {"forward"},
            .preferred_queue = QueueType::Graphics
        };
        return desc;
    }

    void Compile(NodeContext& ctx) override {
        // Create pipeline, load shaders, etc.
        pipeline_ = create_geometry_pipeline();
    }

    void Execute(NodeContext& ctx) override {
        auto& encoder = ctx.command_encoder();
        encoder.begin_render_pass(/* ... */);
        encoder.bind_pipeline(pipeline_);
        encoder.draw_indexed(geometry_.index_count);
        encoder.end_render_pass();
    }

private:
    Pipeline pipeline_;
    GeometryData geometry_;
};
```

### Example 2: Post-Process Compute Node

```cpp
class BloomNode : public INode {
public:
    const NodeDescriptor& Reflect() const override {
        static const NodeDescriptor desc = {
            .id = "bloom_filter",
            .creates = {
                {.name = "bloom_output", .kind = ResourceKind::Texture,
                 .format = ResourceFormat::Rgba16f, .width = 960, .height = 540,
                 .transient = true}
            },
            .reads = {
                {.name = "hdr_input", .stage = PipelineStage::Compute,
                 .access = Access::Read, .state = ResourceState::ShaderReadOnly}
            },
            .writes = {},
            .tags = {"post_process", "bloom"},
            .preferred_queue = QueueType::Compute  // Can run async!
        };
        return desc;
    }

    void Compile(NodeContext& ctx) override {
        compute_pipeline_ = create_bloom_pipeline();
    }

    void Execute(NodeContext& ctx) override {
        auto& encoder = ctx.command_encoder();
        auto output = ctx.resource_info("bloom_output");
        
        encoder.bind_pipeline(compute_pipeline_);
        encoder.dispatch_compute(
            (output.width + 15) / 16,
            (output.height + 15) / 16, 1);
    }

private:
    ComputePipeline compute_pipeline_;
};
```

### Example 3: Multi-Resource Lighting Pass

```cpp
class DeferredLightingNode : public INode {
public:
    const NodeDescriptor& Reflect() const override {
        static const NodeDescriptor desc = {
            .id = "deferred_lighting",
            .creates = {
                {.name = "lit_output", .kind = ResourceKind::Texture,
                 .format = ResourceFormat::Rgba16f, .width = 1920, .height = 1080,
                 .transient = true}
            },
            .reads = {
                {.name = "gbuffer_albedo", .stage = PipelineStage::Fragment,
                 .access = Access::Read, .state = ResourceState::ShaderReadOnly},
                {.name = "gbuffer_normal", .stage = PipelineStage::Fragment,
                 .access = Access::Read, .state = ResourceState::ShaderReadOnly},
                {.name = "gbuffer_depth", .stage = PipelineStage::Fragment,
                 .access = Access::Read, .state = ResourceState::ShaderReadOnly}
            },
            .writes = {},
            .tags = {"pbr", "deferred", "lighting"},
            .preferred_queue = QueueType::Graphics
        };
        return desc;
    }

    void Compile(NodeContext& ctx) override {
        lighting_pipeline_ = create_deferred_lighting_pipeline();
    }

    void Execute(NodeContext& ctx) override {
        auto& encoder = ctx.command_encoder();
        
        // Bind G-buffer inputs
        encoder.bind_texture(0, ctx.resource_handle("gbuffer_albedo"));
        encoder.bind_texture(1, ctx.resource_handle("gbuffer_normal"));
        encoder.bind_texture(2, ctx.resource_handle("gbuffer_depth"));
        
        // Fullscreen lighting pass
        encoder.begin_render_pass(/* ... */);
        encoder.bind_pipeline(lighting_pipeline_);
        encoder.draw(3); // Fullscreen triangle
        encoder.end_render_pass();
    }

private:
    Pipeline lighting_pipeline_;
};
```

### Example 4: External Resource Node

```cpp
class SwapchainOutputNode : public INode {
public:
    const NodeDescriptor& Reflect() const override {
        static const NodeDescriptor desc = {
            .id = "swapchain_blit",
            .creates = {},  // Swapchain is external
            .reads = {
                {.name = "final_color", .stage = PipelineStage::Fragment,
                 .access = Access::Read, .state = ResourceState::ShaderReadOnly}
            },
            .writes = {
                {.name = "swapchain", .stage = PipelineStage::Fragment,
                 .access = Access::Write, .state = ResourceState::ColorAttachment}
            },
            .tags = {"output"},
            .preferred_queue = QueueType::Graphics
        };
        return desc;
    }

    void Compile(NodeContext& ctx) override {
        blit_pipeline_ = create_blit_pipeline();
    }

    void Execute(NodeContext& ctx) override {
        auto& encoder = ctx.command_encoder();
        encoder.begin_render_pass(/* ... */);
        encoder.bind_pipeline(blit_pipeline_);
        encoder.bind_texture(0, ctx.resource_handle("final_color"));
        encoder.draw(3); // Fullscreen triangle
        encoder.end_render_pass();
    }

private:
    Pipeline blit_pipeline_;
};
```

## Validation Rules

### Compile-Time Validation

Performed during frame graph plan construction:

1. **Resource Provenance**
   ```cpp
   // ERROR: Node reads resource that doesn't exist
   .reads = {{"unknown_texture", /* ... */}}
   ```
   → Diagnostic: "Node 'my_node' reads resource 'unknown_texture' which is not created by any node"

2. **Write Conflicts**
   ```cpp
   // ERROR: Multiple nodes write to same resource
   NodeA: .writes = {{"shared_buffer", /* ... */}}
   NodeB: .writes = {{"shared_buffer", /* ... */}}
   ```
   → Diagnostic: "Resource 'shared_buffer' has multiple writers: 'node_a', 'node_b'"

3. **Queue Compatibility**
   ```cpp
   // ERROR: Graphics stages on compute-only queue
   .preferred_queue = QueueType::Compute,
   .writes = {{"color_target", .stage = PipelineStage::Fragment, /* ... */}}
   ```
   → Diagnostic: "Node 'my_node' uses fragment stage but prefers compute queue"

4. **Circular Dependencies**
   ```cpp
   // ERROR: A → B → A
   NodeA: .reads = {{"b_output", /* ... */}}
   NodeB: .reads = {{"a_output", /* ... */}}
   ```
   → Diagnostic: "Circular dependency detected: node_a → node_b → node_a"

5. **Format Compatibility**
   ```cpp
   // ERROR: Depth texture used as color attachment
   NodeA: .creates = {{.name = "depth", .format = ResourceFormat::Depth32f}}
   NodeB: .writes = {{.name = "depth", .state = ResourceState::ColorAttachment}}
   ```
   → Diagnostic: "Resource 'depth' format Depth32f incompatible with state ColorAttachment"

### Runtime Validation

Performed during execution (debug builds only):

1. **Resource state tracking** — Verify actual GPU resource states match declared states
2. **Budget monitoring** — Emit warnings when nodes exceed time budgets
3. **Descriptor stability** — Detect if `Reflect()` returns different values across calls

## Hot-Reload Support

### Node Descriptor Changes

When a plugin DLL updates and node descriptors change:

```cpp
// Before reload
const NodeDescriptor& Reflect() const {
    static const NodeDescriptor desc = {
        .creates = {{"output", .format = ResourceFormat::Rgba8Unorm, /* ... */}}
    };
    return desc;
}

// After reload (incompatible change)
const NodeDescriptor& Reflect() const {
    static const NodeDescriptor desc = {
        .creates = {{"output", .format = ResourceFormat::Rgba16f, /* ... */}}  // Changed!
    };
    return desc;
}
```

**Planner Response:**
1. Detect descriptor change via hash comparison
2. Invalidate current frame graph plan
3. Rebuild plan with updated descriptors
4. Validate new plan (may reject if incompatible)
5. Resume execution on next frame

### Compatibility Checks

**Compatible changes** (hot-reload succeeds):
- Adding new tags
- Changing `preferred_queue` (if queue supports operations)
- Adding new optional resources

**Incompatible changes** (hot-reload requires full rebuild):
- Changing resource formats
- Changing resource dimensions
- Removing resources
- Changing read/write access patterns

### Registry Integration

```cpp
// Register node factory
FrameGraphNodeRegistry::instance().register_node(
    "my_custom_node",
    []() -> std::unique_ptr<INode> {
        return std::make_unique<MyCustomNode>();
    },
    /* hot_reload_enabled = */ true
);

// On hot-reload trigger
FrameGraphNodeRegistry::instance().reload_node("my_custom_node");
// → Triggers plan rebuild with new descriptor
```

## Plugin Integration

### Subsystem Plugin Pattern

Nodes can be registered via subsystem plugins:

```cpp
class RenderingSubsystemPlugin : public ISubsystemInterface {
public:
    void initialize(const SubsystemLifecycleContext& ctx) override {
        // Register built-in nodes
        auto& registry = FrameGraphNodeRegistry::instance();
        registry.register_node("gbuffer", []() { 
            return std::make_unique<GBufferNode>(); 
        });
        registry.register_node("lighting", []() { 
            return std::make_unique<LightingNode>(); 
        });
        registry.register_node("post_process", []() { 
            return std::make_unique<PostProcessNode>(); 
        });
    }

    void shutdown() noexcept override {
        // Cleanup handled by registry
    }

    void tick(const SubsystemUpdateContext& ctx) override {
        // Not used for rendering nodes
    }
};
```

### Hot-Reload Events

Plugin lifecycle integration:

```cpp
// Plugin reload notification
void on_plugin_reload(std::string_view plugin_name) {
    // Query updated node descriptors
    auto nodes = FrameGraphNodeRegistry::instance().nodes_from_plugin(plugin_name);
    
    // Rebuild affected plans
    for (auto& node_id : nodes) {
        planner_.invalidate_node(node_id);
    }
    
    // Recompile and validate
    auto result = planner_.rebuild();
    if (result.is_err()) {
        log_error("Hot-reload failed: {}", result.error());
        // Revert to previous plan
        planner_.restore_previous_plan();
    }
}
```

## Implementation Checklist

### Phase 1: Documentation (RG-450-A)
- [x] Document node descriptor structures (this document)
- [x] Define INode interface semantics (Reflect, Compile, Execute)
- [x] Create example node implementations (4 examples)
- [x] Specify validation rules (compile-time and runtime)
- [x] Document hot-reload support and plugin integration
- [ ] Review API with module leads
- [ ] Update parent RG-450 with progress

### Phase 2: Validation Enhancement (RG-450-B)
- [ ] Implement compile-time validation for resource provenance
- [ ] Add write conflict detection
- [ ] Implement queue compatibility checks
- [ ] Add circular dependency detection
- [ ] Create validation error diagnostics with clear messages

### Phase 3: Hot-Reload Support (RG-450-C)
- [ ] Implement descriptor hash comparison
- [ ] Add plan invalidation and rebuild logic
- [ ] Create compatibility checker for descriptor changes
- [ ] Add registry hot-reload event hooks
- [ ] Test plugin reload scenarios

### Phase 4: Example Nodes (RG-450-D)
- [ ] Implement GBufferNode (deferred rendering)
- [ ] Implement DeferredLightingNode (multi-input)
- [ ] Implement BloomNode (async compute)
- [ ] Implement SwapchainOutputNode (external resource)
- [ ] Add unit tests for each example

### Phase 5: Documentation (RG-450-E)
- [ ] Update rendering module README
- [ ] Create node authoring guide
- [ ] Document tag conventions
- [ ] Add troubleshooting guide
- [ ] Update ADR-0003 with node descriptor details

## Success Criteria

- [ ] Node descriptors fully documented with examples
- [ ] INode interface semantics clearly specified
- [ ] Validation rules comprehensive and testable
- [ ] Hot-reload behavior deterministic and safe
- [ ] Plugin integration pattern documented
- [ ] 4+ example nodes demonstrating common patterns
- [ ] API design reviewed and approved by rendering-lead, runtime-lead, tools-lead

## Open Questions

1. **Dynamic viewport sizing:** Should nodes support runtime viewport changes without rebuild?
   - **Decision:** Phase 1 uses static descriptors; Phase 2 may add viewport scaling hints

2. **Cross-frame resource persistence:** How should history buffers (TAA, reprojection) work?
   - **Decision:** `transient = false` reserves across frames; planner maintains handle stability

3. **Conditional resource creation:** Can nodes create resources based on runtime conditions?
   - **Decision:** No; descriptors must be static. Use tags for profile-based inclusion/exclusion

4. **Multi-view rendering (VR):** How do nodes express stereo rendering requirements?
   - **Decision:** Deferred to VR-specific design; may use `array_layers` or special tags

## References

- [ADR-0003: Runtime Frame-Graph Contract](../specs/ADR_0003_RUNTIME_FRAME_GRAPH.md)
- [Frame Graph Node Header](../../engine/rendering/include/engine/rendering/frame_graph_node.hpp)
- [Frame Graph Types Header](../../engine/rendering/include/engine/rendering/frame_graph_types.hpp)
- [Plugin Architecture Design](PLUGIN_ARCHITECTURE.md)
- [RG-450 Parent Task](../../hybrid_workflow/backlog/RG-450-modular-render-pipeline.md)
- [RG-450-A Subtask](../../hybrid_workflow/backlog/RG-450-A-node-descriptor-api.md)

