# Compute Module

## Current State
- Supplies a polymorphic `engine::compute::Dispatcher` interface with factory helpers `make_cpu_dispatcher()` and `make_cuda_dispatcher()`. Both implementations share a common dependency graph core while timing kernel execution so callers can swap backends without rewriting scheduling logic.
- Provides math helpers such as `identity_transform()` and default CUDA device transform/axis shims for downstream GPU consumers.
- Optionally builds the CUDA companion target (`engine_compute_cuda`) when `ENGINE_ENABLE_CUDA=ON`, letting host code stage GPU integration without duplicating device metadata while keeping CPU-only builds lean by default.
- Smoke tests in `engine/compute/tests/` verify module registration.

## Usage
- Build the module with `cmake --build --preset <preset> --target engine_compute`; pass `-DENGINE_ENABLE_CUDA=ON` during configuration to compile the CUDA companion target and `-DENGINE_ENABLE_COMPUTE_CUDA=OFF` to disable its integration while still building it.
- Include `<engine/compute/api.hpp>` and obtain a dispatcher via `auto dispatcher = engine::compute::make_cpu_dispatcher();` or `make_cuda_dispatcher()` depending on the desired backend. The returned object follows the same interface so dependency graphs and kernel registration code remain identical across CPU and CUDA pathways.
- Include `<engine/compute/cuda/api.hpp>` when requesting default GPU metadata.
- Run `ctest --preset <preset> --tests-regex engine_compute` after enabling testing to confirm dispatcher behaviour for both implementations.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
