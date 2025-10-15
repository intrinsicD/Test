# Compute Module

## Current State
- Supplies a CPU-side `KernelDispatcher` that records kernels with dependency edges, executes them in topological order, and produces execution reports with per-kernel timing.
- Provides math helpers such as `identity_transform()` and default CUDA device transform/axis shims for downstream GPU consumers.
- Optionally builds the CUDA companion target (`engine_compute_cuda`) when `ENGINE_ENABLE_CUDA=ON`, letting host code stage GPU integration without duplicating device metadata while keeping CPU-only builds lean by default.
- Smoke tests in `engine/compute/tests/` verify module registration.

## Usage
- Build the module with `cmake --build --preset <preset> --target engine_compute`; pass `-DENGINE_ENABLE_CUDA=ON` during configuration to compile the CUDA companion target and `-DENGINE_ENABLE_COMPUTE_CUDA=OFF` to disable its integration while still building it.
- Include `<engine/compute/api.hpp>` to orchestrate CPU kernels and `<engine/compute/cuda/api.hpp>` when requesting default GPU metadata.
- Run `ctest --preset <preset> --tests-regex engine_compute` after enabling testing to confirm dispatcher behaviour.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
