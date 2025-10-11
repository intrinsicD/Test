# Compute Module

## Current State

- Hosts a CPU kernel dispatcher capable of enforcing dependency order and emitting execution traces.
- Exposes a minimal math helper (`identity_transform`) shared across GPU and CPU code paths.

## Usage

- Link against `engine_compute` to enqueue kernels and dispatch them via the topological scheduler; the target inherits `engine::project_options` and contributes headers to `engine::headers` alongside `engine_compute_cuda`.
- Extend the dispatcher or integrate GPU execution paths under the CUDA submodule.
- Configure with the canonical presets (for example `cmake --preset linux-gcc-debug`) and run smoke tests via `ctest --preset linux-gcc-debug` when iterating.

## TODO / Next Steps

The dispatcher is currently limited to CPU execution. The roadmap below sequences
the work required to deliver heterogeneous (CPU + GPU) dispatch that can plug
into the broader runtime scheduler.

1. **Unify the dispatch abstractions.**
   - Formalise a backend-agnostic command description that can be consumed by
     both CPU and GPU executors.
   - Extend `KernelDispatcher` with explicit resource lifetime metadata so that
     dependency tracking is no longer implicit in the user-defined callbacks.
   - Document the public API changes under `engine/compute/include` once
     stabilised.
2. **Introduce GPU execution backends.**
   - Stand up a CUDA-backed executor that consumes the unified command format
     and mirrors the CPU topological scheduler.
   - Surface device/stream management hooks so the runtime can provide shared
     pools and avoid redundant initialisation.
   - Instrument both CPU and GPU paths with lightweight timing markers to feed
     profiling tools.
3. **Integrate with the runtime scheduler.**
   - Expose submission queues that the runtime frame graph can drive, including
     synchronisation primitives for cross-backend dependencies.
   - Author regression scenarios in `engine/compute/tests` that exercise mixed
     CPU/GPU graphs and validate determinism of the reported execution order.
   - Produce usage samples under `docs/` once the runtime integration settles.
