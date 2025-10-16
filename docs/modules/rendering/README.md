# Rendering Module

## Current State
- Provides a declarative frame graph API for describing render passes, resources, and dependencies, alongside a forward rendering pipeline implementation.
- Implements command encoder abstractions, resource providers (including a recording GPU resource provider), and a material system coordinating shader bindings.
- Defines backend interfaces and scheduler stubs so platform-specific renderers can plug in while sharing common orchestration.
- Extensive tests in `engine/rendering/tests/` cover frame graph construction, backend adapters, and forward pipeline behaviour.
- Frame graph resources carry explicit format, dimension, usage, and state metadata, and render passes publish queue affinity hints that schedulers consume when selecting submission queues.

## Usage
- Build via `cmake --build --preset <preset> --target engine_rendering`; this pulls in core, assets, platform, and scene dependencies.
- Include `<engine/rendering/frame_graph.hpp>`, `<engine/rendering/forward_pipeline.hpp>`, or `<engine/rendering/material_system.hpp>` when constructing render workloads.
- Execute `ctest --preset <preset> --tests-regex engine_rendering` to verify frame graph scheduling and pass correctness after changes.

## Frame Graph Metadata and Queue Affinity

- Use `FrameGraphResourceDescriptor` when calling `FrameGraph::create_resource`. Populate:
  - `format` (`ResourceFormat`) and `dimension` (`ResourceDimension`) so GPU resource providers can allocate concrete textures or buffers.
  - `usage` (`ResourceUsage`) to describe how passes will access the resource; combine flags with `operator|` and query them with `has_flag`.
  - `initial_state` / `final_state` (`ResourceState`) to document the expected layout before the first pass and after the last pass that touches the resource.
- `FrameGraphResourceInfo` now exposes the same metadata to passes during execution and to `IGpuResourceProvider::on_transient_{acquire,release}`.
- `RenderPass` exposes `set_queue` and an optional constructor parameter so passes can declare their queue affinity; schedulers receive the preferred queue alongside the pass reference when making routing decisions.
- `engine::rendering::CallbackRenderPass` accepts the preferred queue as its fourth constructor argument for concise setup of compute or transfer passes.

## Migration Notes

1. Replace calls to `FrameGraph::create_resource("Name")` with `FrameGraph::create_resource(descriptor)` and populate all descriptor fields for transient resources.
2. Update custom render passes to either forward the preferred queue through the base-class constructor or call `set_queue` inside their constructor.
3. Extend tests to assert the propagated metadata using `FrameGraph::resource_info` or the recording GPU resource provider to catch regressions when adding new passes.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
