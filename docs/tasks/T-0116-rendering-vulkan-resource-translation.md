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
   - Provide a deterministic translation layer (documented) that prepares create-info payloads for a future Vulkan allocator;
     actual GPU allocation remains out-of-scope for this iteration.
   - Provide barrier translation for initial/final states using Vulkan pipeline stages and access masks.
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
   - Provide a minimal sample (documentation snippet + unit test reference) demonstrating configuration and submission path.
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
- [x] Vulkan translation layer emits deterministic `VkImage*`/`VkBuffer*` create info from frame-graph metadata, providing the
      data required for future allocation work.
- [x] Tests cover resource translation and barrier mapping; suites execute under CI-friendly presets.
- [x] Documentation updates land in rendering README and root README with clear setup steps.
- [x] Sample usage documented via README snippet and exercised in `engine_rendering_tests`.
- [x] Roadmap `RT-003` remaining checkboxes can be marked complete.

## Metrics & Benchmarks
- Translation exercised by `engine_rendering_tests` (`test_vulkan_resource_translation.cpp`) verifying color/depth/buffer
  descriptors and barrier mappings.
- Barrier translation validated against graphics/compute/transfer stage combinations; no runtime Vulkan validation layer errors
  are emitted because the layer operates on metadata without touching a real device.

## Follow-Up
- Integrate the translation layer with a real Vulkan allocator/command encoder once device access is available, and capture
  validation-layer output to confirm the metadata contracts against actual drivers.

## Open Questions
- Do we require VMA or a custom allocator? Evaluate trade-offs and document decision.
- How do we manage descriptor set recycling across frames to avoid leaks?
- Should we gate Vulkan tests behind feature detection or provide mock layer fallbacks?
