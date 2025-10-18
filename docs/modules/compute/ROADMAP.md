# Compute Module Roadmap

_Last Updated: 2025-02-17 (Sprint 06 architecture/task audit)_

## Near Term (`AI-004`)
- [x] Add dependency cycle detection and diagnostic reporting to `KernelDispatcher` to guard against invalid job graphs during development. The dispatcher now surfaces DOT graphs through `DependencyGraph::to_dot()` so developers can inspect scheduling issues without running kernels.
- [ ] Integrate a configurable clock abstraction so execution reports can capture CPU vs GPU timing domains consistently.

## Mid Term
- Implement a thread pool backed executor that parallelises independent kernels while respecting dependency edges; expose controls for worker counts and scheduling policies.
- Flesh out the CUDA companion to manage device selection, stream lifetimes, and host/device synchronisation primitives shared with rendering.

## Long Term
- Unify the compute dispatcher with the runtime job graph so animation, physics, and rendering passes can schedule heterogeneous workloads through a common interface.
