# Compute Module Roadmap

## Near Term
- Add dependency cycle detection and diagnostic reporting to `KernelDispatcher` to guard against invalid job graphs during development.
- Integrate a configurable clock abstraction so execution reports can capture CPU vs GPU timing domains consistently.

## Mid Term
- Implement a thread pool backed executor that parallelises independent kernels while respecting dependency edges; expose controls for worker counts and scheduling policies.
- Flesh out the CUDA companion to manage device selection, stream lifetimes, and host/device synchronisation primitives shared with rendering.

## Long Term
- Unify the compute dispatcher with the runtime job graph so animation, physics, and rendering passes can schedule heterogeneous workloads through a common interface.
