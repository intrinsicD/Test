# T-0116: Vulkan Resource Translation & Documentation

## Goal
Complete the outstanding `RT-003` tasks by implementing Vulkan resource translation layers for frame-graph handles and authoring
backend configuration documentation and samples.

## Background
- Roadmap alignment: [`RT-003`](../ROADMAP.md#rt-003-vulkan-backend-prototype) has two unchecked deliverables.
- Current state: Vulkan scheduler prototype exists (per `T-0104`); resource providers still use placeholder handles and
  documentation is limited to internal notes.
- Dependencies: Frame-graph metadata expansion (`AI-003`) is complete, providing the descriptors required for Vulkan allocations.

## Inputs
- Code: `engine/rendering/backends/vulkan/`, `engine/rendering/resources/`, `engine/rendering/tests/`,
  `engine/runtime/runtime_host.cpp`, example apps under `engine/samples/` (if available).
- Docs: `docs/modules/rendering/README.md`, `docs/modules/runtime/README.md`, `docs/ROADMAP.md`, `README.md`.
- Build assets: CMake configuration for Vulkan backend, existing samples in `examples/` tree.

## Constraints
- Abide by Vulkan validation layer requirements: explicit resource state transitions, descriptor set management, and memory
  lifetime.
- Maintain backend-agnostic interfaces—no Vulkan headers should leak outside backend implementation files.
- Keep samples headless-friendly where possible; provide conditional execution when a surface is unavailable.
- Tests must run without requiring GPU swapchain presentation (use mock surface or off-screen framebuffer).

## Deliverables
1. **Resource Translation Layer**
   - Map `FrameGraphResourceInfo` descriptors to Vulkan `VkImageCreateInfo` / `VkBufferCreateInfo` structures.
   - Implement allocation via VMA or manual allocation strategy (document choice) with lifetime tied to frame-graph resources.
   - Provide barrier translation for initial/final states using Vulkan pipeline barriers.
2. **Descriptor & Pass Integration**
   - Ensure render passes specify queue/command buffer usage, translating to Vulkan queue submissions.
   - Update scheduler to bind translated resources to command buffers and descriptor sets.
3. **Testing**
   - Extend `engine/rendering/tests` with Vulkan-backed tests verifying resource creation, state transitions, and destruction.
   - Add integration test verifying runtime submits a pass that reads/writes Vulkan resources without validation errors (run under
     `VK_LAYER_KHRONOS_validation`).
4. **Documentation & Samples**
   - Expand rendering README with Vulkan setup, feature flags, and troubleshooting.
   - Update root README build instructions with Vulkan requirements.
   - Provide a minimal sample (`engine/samples/vulkan_triangle` or adapt existing sample) demonstrating configuration and
     submission path.
5. **Telemetry & Diagnostics**
   - Document how to enable validation layers and interpret errors; optionally log validation output to existing telemetry tools.
6. **Follow-Up Tracking**
   - Identify gaps for other backends (DirectX12, Metal) and note them in roadmap/module README.

## Work Breakdown
1. Implement resource translation helpers and integrate with scheduler.
2. Add descriptor/queue bindings and ensure compatibility with frame-graph metadata.
3. Author tests and integration coverage.
4. Create/update documentation and samples.
5. Capture validation results and append summary to this task file.

## Acceptance Criteria
- [ ] Vulkan backend allocates and binds resources based on frame-graph metadata without validation errors.
- [ ] Tests cover resource creation/destruction and run in CI (headless/off-screen).
- [ ] Documentation updates land in rendering README and root README with clear setup steps.
- [ ] Sample application demonstrates configuring Vulkan backend end-to-end.
- [ ] Roadmap `RT-003` remaining checkboxes can be marked complete.

## Metrics & Benchmarks
- Record resource creation/destruction timings and memory footprint for representative frame graphs.
- Track validation layer output—must report zero errors/warnings for provided scenarios.

## Open Questions
- Do we require VMA or a custom allocator? Evaluate trade-offs and document decision.
- How do we manage descriptor set recycling across frames to avoid leaks?
- Should we gate Vulkan tests behind feature detection or provide mock layer fallbacks?
