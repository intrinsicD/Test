# Kernel Dispatcher Extension Guide

This guide explains how to extend `engine::compute::Dispatcher` with custom
kernels, integrate dependency metadata, and publish telemetry that other
subsystems (runtime, diagnostics, tooling) consume. It fulfils roadmap item
`CO-141` and should be read together with the public API in
[`engine/compute/api.hpp`](../../../engine/compute/include/engine/compute/api.hpp).

## Architectural Overview

The dispatcher exposes a strict interface around three responsibilities:

1. **Kernel registration** — `Dispatcher::add_kernel` stores a callable and the
   identifiers of kernels that must run beforehand.
2. **Scheduling & execution** — `Dispatcher::dispatch` topologically orders
   registered kernels, executes each callback, and records timing data.
3. **Telemetry** — `ExecutionReport` captures the execution order, individual
   durations, the active clock domain, and the dependency graph that was used
   for scheduling.

Two concrete implementations exist today:

- `make_cpu_dispatcher()` — always available; measures wall-clock time using the
  steady clock by default.
- `make_cuda_dispatcher()` — compiled when `ENGINE_ENABLE_COMPUTE_CUDA` is set
  and `dispatcher_capabilities().cuda_available` returns `true`.

Consumers should call `dispatcher_capabilities()` or the individual
`is_cpu_dispatcher_available()` / `is_cuda_dispatcher_available()` helpers to
select an implementation at runtime. Always provide a CPU fallback for CI and
headless environments.

## Registering Kernels and Declaring Dependencies

`Dispatcher::add_kernel` returns a stable `kernel_id` that can be used as a
dependency by future registrations:

```cpp
auto dispatcher = engine::compute::make_cpu_dispatcher();
const auto preprocess = dispatcher->add_kernel(
    "preprocess",
    [] {
        // Prepare data shared by downstream kernels.
    });
const auto simulate = dispatcher->add_kernel(
    "simulate",
    [] {
        // Perform expensive work.
    },
    {preprocess});
```

Guidelines:

- Register kernels in a deterministic order; identifiers are assigned
  sequentially starting at zero.
- Avoid capturing non-thread-safe state in callbacks unless higher-level
  synchronisation is guaranteed.
- Dependencies must refer to kernels that were already registered. The
  dispatcher validates registrations immediately and throws
  `std::runtime_error` with `KernelDispatcher detected a cycle` if a cyclic
  dependency is introduced.
- Supplying an identifier that is greater than or equal to the current kernel
  count is treated as unresolved. Dispatch will then throw `std::out_of_range`
  describing the missing identifiers. Use `DependencyGraph::to_dot()` to render
  a Graphviz diagram that visualises the offending edges.

Reset state between frames using `Dispatcher::clear()` when scenarios require a
fresh dependency graph.

## Clock Configuration and Telemetry

Each dispatcher owns a `ClockConfiguration` describing how kernel durations are
measured. The default value returned by `make_steady_clock_configuration()`
uses `std::chrono::steady_clock` in the CPU domain. Override the clock when
integrating GPU timers or external profiling tools:

```cpp
engine::compute::ClockConfiguration gpu_clock{};
gpu_clock.name = "cuda_events";
gpu_clock.domain = engine::compute::TimingDomain::Gpu;

const auto measure_kernel = [&device_timer] (const engine::compute::kernel_callback& callback) {
    return device_timer.measure(callback);
};
gpu_clock.measure = measure_kernel;

dispatcher->set_clock(std::move(gpu_clock));
```

`set_clock` validates that `measure` is callable and throws
`std::invalid_argument` otherwise. The active configuration is reported via
`Dispatcher::clock_configuration()` and copied into the `ExecutionReport` so
observability layers can tag metrics with the correct clock name and domain.

The `ExecutionReport` returned by `dispatch()` includes:

- `execution_order` — kernel names in the order they ran.
- `kernel_durations` — duration for each executed kernel, indexed to match the
  execution order.
- `clock_name` / `clock_domain` — metadata describing the clock used during the
  run.
- `dependency_graph` — the graph that was executed, enabling diagnostics to
  persist scheduling artefacts alongside timing data.

Publish the report to telemetry sinks or regression dashboards as part of
`AI-004` work.

## Runtime Integration Checklist

To integrate new kernels safely:

1. **Capability probe** — decide on CPU vs. CUDA dispatcher during subsystem
   initialisation based on `dispatcher_capabilities()`.
2. **Register kernels** — add kernels during subsystem bootstrap. Preserve the
   returned identifiers for dependency wiring and future hot-reload support.
3. **Validate dependencies** — treat exceptions thrown by `add_kernel` and
   `dispatch` as fatal configuration errors; surface the full DOT graph via
   logs to simplify debugging.
4. **Capture telemetry** — consume the `ExecutionReport` to populate metrics or
   expose diagnostics via the tooling module.
5. **Clean up** — call `clear()` before rebuilding the graph when kernels or
   dependency chains change at runtime.

## Testing Guidance

- Unit test individual kernels and their callbacks in isolation.
- Add dispatcher-focused tests that register representative dependency graphs
  and assert that `ExecutionReport` matches expectations.
- Exercise failure modes: register a deliberate cycle to confirm the thrown
  error and verify that unresolved dependencies trigger the `std::out_of_range`
  path.
- When GPU dispatchers are available, run the same suite under both CPU and
  CUDA configurations to keep parity.

Following this checklist keeps the dispatcher extensible for future runtime and
async streaming work without reintroducing cyclic scheduling bugs or losing
observability guarantees.
