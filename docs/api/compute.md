# Compute Module

The compute API implements a CPU-oriented kernel dispatcher that mirrors the dependency management used by GPU
command graphs. Headers live in [`engine/compute/api.hpp`](../../engine/compute/include/engine/compute/api.hpp).

## KernelDispatcher

`KernelDispatcher` records a set of named kernels alongside the indices of the kernels they depend on. When
`dispatch()` is called the dispatcher performs a Kahn topological sort, executes each kernel exactly once, and returns
an `ExecutionReport` containing the realised execution order.

```cpp
#include <engine/compute/api.hpp>

engine::compute::KernelDispatcher dispatcher;
const auto prepare = dispatcher.add_kernel("prepare", [] {
    // upload data, allocate buffers, etc.
});
dispatcher.add_kernel("simulate", [] {
    // run the heavy work
}, {prepare});
const auto report = dispatcher.dispatch();
```

If a dependency index is out of range or if a cycle is detected during dispatch, the dispatcher raises an exception to
prevent inconsistent runtime state. The runtime module uses the report to expose kernel ordering through the C API.

_Last updated: 2025-03-15_
