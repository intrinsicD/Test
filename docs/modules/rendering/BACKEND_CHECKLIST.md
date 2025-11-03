# Rendering Backend Checklist

This checklist distils the requirements for exercising the Vulkan and OpenGL GPU
schedulers that back the rendering module initiatives (`RT-003`, `RE-540`). Use
it to bootstrap environments, configure the engine, and verify parity between
runtime submission paths and backend schedulers.

## Shared Prerequisites

- **Toolchain** – Modern C++20 compiler (Clang ≥ 22, GCC ≥ 12, MSVC ≥ 19.34) and
  CMake ≥ 3.20 as captured in the root workspace documentation. Ninja is the
  default generator for the supplied presets.
- **Rendering + Platform toggles** – Configure CMake with
  `-DENGINE_ENABLE_RENDERING=ON` and `-DENGINE_ENABLE_PLATFORM=ON`. The presets
  under `scripts/build/presets/` already enable these flags.
- **Tests** – Build `engine_rendering` and `engine_rendering_tests` targets when
  exercising backend schedulers. Pair the backend under test with
  `backend::vulkan::VulkanGpuResourceProvider` for deterministic Vulkan handle
  lifecycles, or fall back to `resources::RecordingGpuResourceProvider` when
  capturing translation traces without binding to real driver APIs.

## Vulkan Backend

### Prerequisites

- **Vulkan SDK 1.3.x** – Install the latest 1.3 series SDK from LunarG. Ensure
  `VULKAN_SDK` is exported and `VK_LAYER_PATH` points at the validation layer
  manifest directory. Enable `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation`
  during local runs to surface contract violations early.
- **Platform drivers** – Install GPU drivers that match the SDK. Headless CI can
  rely on the Vulkan stub implementation shipped in
  [`vulkan_stub.hpp`](../../../engine/rendering/include/engine/rendering/backend/vulkan/vulkan_stub.hpp).

### Configure the Build

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

### Runtime Submission Flow

- Instantiate `engine::runtime::RuntimeHost` and advance it with `tick()` to
  populate animation, physics, and geometry state.
- Create a submission context via
  `engine::rendering::RuntimeSubmissionContext` (aliased as
  `RuntimeHost::RenderSubmissionContext`) that wires the material system,
  resource providers, Vulkan GPU scheduler, command encoder provider, and
  frame graph. The runtime test
  [`RuntimeHost.SubmitsRenderGraphThroughVulkanScheduler`](../../../engine/runtime/tests/test_module.cpp)
  demonstrates the expected wiring and validates resource acquisitions.
- Use `backend::vulkan::VulkanGpuResourceProvider` when exercising the Vulkan
  scheduler to mirror transient resource lifetimes and command buffer recycling
  without requiring device-level allocations; telemetry still reflects resource
  residency for diagnostics and retention tuning.
- Ensure materials and meshes are registered with the render resource provider
  prior to submission, as shown in the same test case.

### Validation & Testing

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

### Telemetry & Troubleshooting

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
- Leverage the backend validation telemetry metrics (`rendering.backend.*`) to
  confirm parity status for each scheduler in dashboards and diagnostics runs.

## OpenGL Backend

### Prerequisites

- **OpenGL 4.5 driver** – Install platform drivers providing OpenGL 4.5 core
  functionality (Mesa, NVIDIA, AMD, or platform-specific SDKs). A headless
  driver or OSMesa build is sufficient for CI; the adapter tests operate on
  translated handles without requiring a live context.
- **Platform surface** – The platform module (GLFW/Mock) supplies windowing.
  For smoke tests a mock surface is acceptable; presentation flows still require
  a concrete window backend.

### Configure the Build

1. Configure CMake with rendering/platform enabled. The default presets already
   provide suitable flags:

   ```bash
   cmake --preset linux-gcc-debug
   cmake --build --preset linux-gcc-debug --target engine_rendering_tests
   ```

   Set `ENGINE_WINDOW_BACKEND=MOCK` when no swap chain is required.
2. No additional SDK integration is necessary; the scheduler translates frame
   graph metadata into OpenGL submission structures using
   `RecordingGpuResourceProvider`.

### Validation & Testing

- Run the backend adapter tests that exercise queue normalisation and barrier
  translation:

  ```bash
  ctest --preset linux-gcc-debug --tests-regex BackendAdapters.OpenGL
  ```

- `BackendAdapters.OpenGLSchedulerRecordsGraphicsQueue` validates semaphore and
  fence translation alongside the computed `glMemoryBarrier` mask stored in
  `OpenGLSubmission::begin_barriers`/`end_barriers`.
- `BackendAdapters.OpenGLSchedulerNormalisesQueueSelections` asserts that
  compute/transfer passes are coerced to the graphics queue, mirroring the
  single-queue OpenGL command stream.
- `BackendAdapters.OpenGLSchedulerDispatchesCommandStream` ensures translated
  submissions are forwarded to the overridable command stream in the expected
  order so driver integrations can record synchronisation behaviour deterministically.
- `RuntimePresentationBackend.OpenGLBackendExecutesFrameGraph` drives the new
  presentation backend through `RuntimeHost`, validating mesh resolution,
  material registration, and frame-graph execution without requiring a live
  OpenGL context.

### Diagnostics & Troubleshooting

- Inspect `backend::opengl::OpenGLSubmission` instances produced by
  `RecordingGpuResourceProvider` to verify `memory_barrier_mask` values. The
  inline constants (for example,
  `backend::opengl::shader_image_access_barrier_bit`) map directly to the
  `glMemoryBarrier` bitfield.
- Override `backend::opengl::CommandStream` to surface driver calls or frame
  capture hooks. The default implementation issues `glMemoryBarrier`/`glFlush`
  calls (when GLAD is available) so headless environments still observe barrier
  ordering without an OpenGL context.
- When integrating with runtime submission flows, capture telemetry counters via
  `backend::validation::backend_parity_metrics` to ensure OpenGL parity remains
  aligned with the Vulkan baseline.
- The runtime smoke test `RuntimeHost.SubmitsRenderGraphThroughOpenGLScheduler`
  exercises queue normalisation end-to-end; use it when validating telemetry
  integration with the runtime submission harness.
