# Compute Module

## Current State

- Hosts a CPU kernel dispatcher capable of enforcing dependency order and emitting execution traces.
- Exposes a minimal math helper (`identity_transform`) shared across GPU and CPU code paths.

## Usage

- Link against `engine_compute` to enqueue kernels and dispatch them via the topological scheduler; the target inherits `engine::project_options` and contributes headers to `engine::headers` alongside `engine_compute_cuda`.
- Extend the dispatcher or integrate GPU execution paths under the CUDA submodule.
- Configure with the canonical presets (for example `cmake --preset linux-gcc-debug`) and run smoke tests via `ctest --preset linux-gcc-debug` when iterating.

## TODO / Next Steps

- Implement GPU-backed dispatch and integrate with runtime scheduling.
