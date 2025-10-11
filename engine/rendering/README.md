# Rendering Module

## Current State

- Implements a prototype frame graph and forward pipeline within the native renderer.
- Structures backend, resource, material, and pass directories for platform-specific integrations.
- Defines a portable GPU scheduling interface wired into the frame graph execution.
- Provides a backend-neutral GPU resource provider contract that exposes API-native handles to the schedulers and
  records transient resource lifetimes.

## Usage

- Link against `engine_rendering` to access render pass orchestration and frame graph primitives; the target inherits `engine::project_options` and shares headers via `engine::headers`.
- Add backend-specific code under `backend/` and keep resource and material definitions in sync.
- Exercise the renderer with the standard presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to ensure dependencies resolve consistently across toolchains.

## TODO / Next Steps

Consult [docs/rendering/ROADMAP.md](../../docs/rendering/ROADMAP.md) for the detailed roadmap. Immediate priorities are:

1. **Short-term – unblock backend integration**
   - Extend frame-graph resource descriptions with formats, dimensions, and usage flags so resource providers can allocate GPU objects deterministically.
   - Propagate queue/command-buffer metadata through render passes and the execution context to prepare for scheduler translation.
   - Prototype a reference GPU scheduler that replays compiled frame graphs into an abstract command encoder for validation.
2. **Mid-term – backend hookups**
   - Implement resource providers and schedulers for Vulkan and DirectX 12 using the enriched descriptors.
   - Build a library of core passes (geometry, lighting, post-processing) that exercises backend integrations and defines explicit dependencies.
3. **Long-term – robustness and tooling**
   - Add stress tests under `engine/rendering/tests` to validate resource lifetime management and synchronization.
   - Integrate profiling hooks and author documentation/samples that demonstrate pass registration and backend configuration.
