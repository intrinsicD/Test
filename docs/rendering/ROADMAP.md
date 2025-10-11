# Rendering Module Roadmap

## Context Review

The rendering module currently provides a CPU-side frame graph and a forward pipeline prototype (`engine/rendering/frame_graph.hpp`, `engine/rendering/forward_pipeline.hpp`) that orchestrate render passes without binding to a concrete GPU backend. Resource abstractions (`engine/rendering/resources`) and the GPU scheduler interface (`engine/rendering/gpu_scheduler.hpp`) expose contracts, but no backend-specific implementations ship yet. Materials, lighting, and visibility subsystems are scaffolded through headers and documentation-only stubs.

## Observations

- The frame graph builds read/write dependencies and produces an execution order, but the compiled plan is not yet translated into backend command buffers.
- `IGpuScheduler` and `resources::IGpuResourceProvider` are pure interfaces; the runtime lacks concrete implementations for DirectX 12, Vulkan, Metal, or OpenGL.
- Resource descriptions expose names and lifetimes yet omit format/usage metadata required for backend allocation and barrier translation.
- Render pass definitions do not advertise the queue/command-buffer semantics that backend schedulers will require to batch work.
- There are no module-level validation tests that exercise the frame graph compilation or verify resource lifetime rules under realistic workloads.

## Proposed Next Steps

### 1. Short-Term (Unblock backend integration)

1. **Resource Description Extensions** – Extend `FrameGraphResourceInfo` and creation APIs to describe formats, dimensions, and usage flags so providers can materialize buffers, textures, and samplers.
2. **Command Context Plumbing** – Ensure each `RenderPass` can describe queue affinity, command-buffer requirements, and synchronization hints; mirror this in `FrameGraphPassExecutionContext`.
3. **GPU Scheduler Prototype** – Implement a reference scheduler that converts compiled passes into a linear submission stream calling into an abstract command encoder. Use it to validate the frame graph logic with stub encoders.

### 2. Mid-Term (Backend hookups)

1. **Resource Provider Implementations** – Provide Vulkan and DirectX 12 resource providers that translate the enriched descriptions into API-native allocations and bind them to frame graph handles.
2. **Backend-Specific Schedulers** – Implement scheduler specializations per backend that translate the reference submission stream into actual API command buffers and queues.
3. **Render Pass Library** – Author foundational passes (geometry, lighting, post-processing) that exercise the backend implementations and define data dependencies explicitly.

### 3. Long-Term (Robustness and tooling)

1. **Validation & Testing** – Add unit/integration tests under `engine/rendering/tests` that stress the frame graph with branching dependencies, transient lifetimes, and backend mock objects.
2. **Profiling & Instrumentation** – Integrate GPU timing queries and CPU profiling scopes to expose pass-level performance metrics.
3. **Documentation & Samples** – Publish backend-specific setup guides, shader samples, and walkthroughs demonstrating how to register new passes and materials.

## Dependencies and Coordination

- Coordinate resource description changes with the assets module to align on material/shader metadata.
- Collaborate with the platform module to ensure windowing and swap-chain abstractions expose the surfaces required by the backend implementations.
- Update the aggregated workspace backlog once the short-term milestones are complete to reflect progress toward backend enablement.
