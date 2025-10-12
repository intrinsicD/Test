# Compute Module

## Current State
- Supplies a CPU-side `KernelDispatcher` that records kernels with dependency edges, executes them in topological order, and produces execution reports with per-kernel timing.
- Provides math helpers such as `identity_transform()` and default CUDA device transform/axis shims for downstream GPU consumers.
- Links against the CUDA submodule target (`engine_compute_cuda`) so host code can stage GPU integration without duplicating device metadata.
- Smoke tests in `engine/compute/tests/` verify module registration.

## Usage
- Build the module with `cmake --build --preset <preset> --target engine_compute`; this also builds the CUDA companion when enabled.
- Include `<engine/compute/api.hpp>` to orchestrate CPU kernels and `<engine/compute/cuda/api.hpp>` when requesting default GPU metadata.
- Run `ctest --preset <preset> --tests-regex engine_compute` after enabling testing to confirm dispatcher behaviour.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
