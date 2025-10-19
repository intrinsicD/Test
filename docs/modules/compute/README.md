# Compute Module

## Current State
- Supplies a polymorphic `engine::compute::Dispatcher` interface with factory helpers `make_cpu_dispatcher()` and `make_cuda_dispatcher()`. Both implementations share a common dependency graph core while timing kernel execution so callers can swap backends without rewriting scheduling logic.
- Exposes `dispatcher_capabilities()` and helper predicates so hosts can query whether the CUDA dispatcher is linked into the current build before attempting to construct it.
- Provides math helpers such as `identity_transform()` and default CUDA device transform/axis shims for downstream GPU consumers.
- Optionally builds the CUDA companion target (`engine_compute_cuda`) when `ENGINE_ENABLE_CUDA=ON`, letting host code stage GPU integration without duplicating device metadata while keeping CPU-only builds lean by default. Dedicated presets (`linux-gcc-debug-cuda`, `windows-msvc-release-cuda`, etc.) flip the flag automatically.
- Smoke tests in `engine/compute/tests/` verify module registration.
- Maintains explicit dependency metadata for every registered kernel, emits GraphViz-compatible diagnostics through `DependencyGraph::to_dot()`, and rejects cyclic registrations before dispatch.
- Execution reports expose the active timing clock name and domain (CPU, GPU, or unknown), and dispatchers accept configurable `ClockConfiguration` instances so hosts can replace the default steady clock with CUDA events or profiling hooks.

## Usage
- Build the module with `cmake --build --preset <preset> --target engine_compute`. Use the `*-cuda` presets (for example `cmake --preset linux-gcc-debug-cuda`) to compile and link the CUDA companion target automatically. Stick with the CPU presets (for example `cmake --preset linux-gcc-debug`) when you want to omit CUDA entirely, or override the presets with `-DENGINE_ENABLE_COMPUTE_CUDA=OFF` if you need the library built but not linked into the runtime.
- Include `<engine/compute/api.hpp>` and obtain a dispatcher via `auto dispatcher = engine::compute::make_cpu_dispatcher();` or `make_cuda_dispatcher()` depending on the desired backend. The returned object follows the same interface so dependency graphs and kernel registration code remain identical across CPU and CUDA pathways. Use `engine::compute::dispatcher_capabilities()` or `is_cuda_dispatcher_available()` to check whether the CUDA backend is available at runtime when builds are configured without CUDA integration.
- Configure timing by passing a custom `ClockConfiguration` to `dispatcher->set_clock(...)`. Execution reports propagate the clock name and `TimingDomain` so telemetry consumers can distinguish CPU timers from GPU event timings.
- Include `<engine/compute/cuda/api.hpp>` when requesting default GPU metadata.
- Run `ctest --preset <preset> --tests-regex engine_compute` after enabling testing to confirm dispatcher behaviour for both implementations.
- Inspect kernel metadata with `Dispatcher::dependency_graph()` or the `ExecutionReport::dependency_graph` field. `DependencyGraph::to_dot()` emits a DOT graph that visualises execution order, resolved edges, and unresolved references, making it easy to diagnose invalid submissions. Registration now fails immediately when a cycle is detected, so reconcile dependency lists before dispatching.

## TODO / Next Steps

- **AI-004:** With the configurable timing abstraction in place, focus on
  the upcoming thread-pool executor tracked in the [compute dependency
  validation plan](../../ROADMAP.md#ai-004-compute-dependency-validation)
  so reports capture parallel dispatch timings alongside the existing
  CPU/GPU domain metadata.
- **DC-002:** Maintain CUDA as an optional dependency by exercising the
  presets and dispatcher parity outlined in the [CUDA optional dependency
  workstream](../../ROADMAP.md#dc-002-cuda-optional-dependency).
