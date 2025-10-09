# Compute Module

## Current State

- Hosts a CPU kernel dispatcher capable of enforcing dependency order and emitting execution traces.
- Exposes a minimal math helper (`identity_transform`) shared across GPU and CPU code paths.

## Usage

- Link against `engine_compute` to enqueue kernels and dispatch them via the topological scheduler.
- Extend the dispatcher or integrate GPU execution paths under the CUDA submodule.

## TODO / Next Steps

- Implement GPU-backed dispatch and integrate with runtime scheduling.
