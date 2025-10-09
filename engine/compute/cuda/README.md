# Engine Compute CUDA

## Current State

- Hosts subdirectories for Public Headers, Sources, Tests.

## Usage

- Link against `engine_compute_cuda` when enabling GPU dispatch; the target inherits `engine::project_options` and its headers join the shared `engine::headers` interface.
- Keep this directory aligned with its parent module and update the README as features land.
- Exercise the CUDA stubs with the canonical presets (e.g., `cmake --preset linux-gcc-debug`) even when kernels remain placeholders to ensure continued integration.

## TODO / Next Steps

- Integrate CUDA kernels and scheduling into the dispatcher.
