# Vulkan Backend Checklist

This checklist distils the requirements for exercising the Vulkan prototype that
backs `RT-003`. Use it to bootstrap environments, configure the engine, and
verify parity between the runtime submission path and the backend scheduler.

## Prerequisites

- **Vulkan SDK 1.3.x** – install the latest 1.3 series SDK from LunarG. Ensure
  `VULKAN_SDK` is exported and `VK_LAYER_PATH` points at the validation layer
  manifest directory. Enable `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation`
  during local runs to surface contract violations early.
- **Platform drivers** – install GPU drivers that match the SDK. Headless CI can
  rely on the Vulkan stub implementation shipped in
  [`vulkan_stub.hpp`](../../../engine/rendering/include/engine/rendering/backend/vulkan/vulkan_stub.hpp).
- **Toolchain** – modern C++20 compiler (Clang ≥ 22, GCC ≥ 12, or MSVC ≥ 19.34)
  and CMake ≥ 3.20, as captured in the root workspace documentation.

## Configure the Build

1. Enable the rendering and platform subsystems when configuring CMake:

   ```bash
   cmake --preset linux-gcc-debug \
     -DENGINE_ENABLE_RENDERING=ON \
     -DENGINE_ENABLE_PLATFORM=ON \
     -DENGINE_WINDOW_BACKEND=GLFW
   ```

   For headless validation (CI, telemetry capture) set
   `ENGINE_WINDOW_BACKEND=MOCK` so the frame graph can execute without a swap
   chain.
2. Link the Vulkan SDK via `VULKAN_SDK` and ensure `glslc`/`spirv-val` are on the
   `PATH` if shader compilation is required.
3. Build the engine and rendering tests:

   ```bash
   cmake --build --preset linux-gcc-debug --target engine_rendering
   cmake --build --preset linux-gcc-debug --target engine_rendering_tests
   ```

## Runtime Submission Flow

- Instantiate `engine::runtime::RuntimeHost` and advance it with `tick()` to
  populate animation, physics, and geometry state.
- Create a `RuntimeHost::RenderSubmissionContext` that wires the material
  system, resource providers, Vulkan GPU scheduler, command encoder provider,
  and frame graph. The runtime test
  [`RuntimeHost.SubmitsRenderGraphThroughVulkanScheduler`](../../../engine/runtime/tests/test_module.cpp)
  demonstrates the expected wiring and validates resource acquisitions.
- Ensure materials and meshes are registered with the render resource provider
  prior to submission, as shown in the same test case.

## Validation & Testing

- Compile the frame graph and execute it through the Vulkan scheduler. The
  rendering test suite exercises resource translation and barrier mapping via
  [`test_vulkan_resource_translation.cpp`](../../../engine/rendering/tests/test_vulkan_resource_translation.cpp).
- Run the focused unit tests:

  ```bash
  ctest --preset linux-gcc-debug --tests-regex engine_rendering_tests
  ctest --preset linux-gcc-debug --tests-regex RuntimeHost.SubmitsRenderGraphThroughVulkanScheduler
  ```

- When validation layers are enabled, the runtime submission test must complete
  without emitting `VK_LAYER_KHRONOS_validation` errors. Failures usually point
  to incorrect resource states or queue selection metadata.

## Telemetry & Troubleshooting

- Inspect `engine::runtime::RuntimeDiagnostics::scene_validation` and stage
  timings to confirm deterministic execution when integrating with the runtime
  host.
- Capture `engine::runtime::RuntimeDiagnostics::frame_graph_serialization` and
  `frame_graph_events` when auditing metadata mismatches; they mirror the
  compiled frame-graph and transient resource lifecycle seen by the runtime.
- Use `python scripts/diagnostics/runtime_frame_telemetry.py` against the debug
  build output to capture submit timings and verify variance stays within the
  `≤ 5%` budget established in the runtime task records.
- If the IO thread pool reports a saturated queue during asset streaming, adjust
  `RuntimeHostDependencies::streaming_config` before submitting the frame graph
  so asset uploads keep pace with rendering.
