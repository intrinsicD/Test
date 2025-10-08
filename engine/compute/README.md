# Compute

_Path: `engine/compute`_

_Last updated: 2025-03-15_


## Overview

The compute subsystem now provides a minimal CPU execution graph that mirrors the structure expected from GPU
schedulers. [`KernelDispatcher`](include/engine/compute/api.hpp) records kernels with dependency information,
performs a Kahn topological traversal, and surfaces an `ExecutionReport` so the runtime can introspect execution
order in tests.

The CUDA subdirectory still hosts backend-specific experiments; the dispatcher is backend-agnostic and can be reused
to prototype task graphs before offloading them to GPUs.

## Usage

```cpp
#include <engine/compute/api.hpp>

engine::compute::KernelDispatcher dispatcher;
const auto prepare = dispatcher.add_kernel("prepare", [] {});
dispatcher.add_kernel("dispatch", [] {}, {prepare});
const auto report = dispatcher.dispatch();
```

## TODO

- Add timing instrumentation to `ExecutionReport` once a high-resolution clock abstraction lands in `engine/core`.
- Support asynchronous completion callbacks for GPU-backed kernels.
- Extend the dependency graph to carry resource state transitions (read/write barriers).
