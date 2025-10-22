# Runtime Module

## Overview

The runtime module orchestrates the engine's main execution loop through `RuntimeHost`, which coordinates animation evaluation, physics simulation, geometry deformation, scene graph updates, and rendering submission. It acts as the integration point for all subsystems and provides comprehensive diagnostics and telemetry.

## Core Concepts

### RuntimeHost

`RuntimeHost` is the primary entry point for managing the engine's lifecycle:

```cpp
#include "engine/runtime/api.hpp"

engine::runtime::RuntimeHostDependencies deps{};
deps.controller = animation::make_linear_controller(animation::make_default_clip());
deps.mesh = geometry::make_unit_quad();
deps.world = physics::PhysicsWorld{};
deps.streaming_config.worker_count = 2;
deps.streaming_config.queue_capacity = 64;

engine::runtime::RuntimeHost host{std::move(deps)};
host.initialize();

// Main loop
while (running) {
    auto state = host.tick(delta_time);
    // Process state.pose, state.bounds, state.body_positions, etc.
}

host.shutdown();
```

### Lifecycle

1. **Construction**: Accept `RuntimeHostDependencies` with animation controllers, physics world, geometry, and subsystem plugins
2. **Initialization**: `initialize()` validates dependencies, starts the IO thread pool, and initializes all registered subsystems
3. **Tick**: `tick(dt)` advances animation, physics, deformation, scene updates, and optionally rendering submission
4. **Shutdown**: `shutdown()` tears down subsystems and the IO thread pool gracefully

### Subsystem Integration

The runtime discovers and manages subsystems through the plugin architecture:

- **Explicit plugins**: Pass subsystem instances via `RuntimeHostDependencies::subsystem_plugins`
- **Automatic discovery**: Use `configure_with_default_subsystems()` to load all compiled subsystems
- **Selective enablement**: Provide subsystem names to `configure_with_default_subsystems(enabled_subsystems)`

The subsystem registry validates dependencies and detects cycles during initialization, emitting `RuntimeError::dependency_cycle` when configuration is invalid.

## Diagnostics & Telemetry

Access runtime metrics through `RuntimeHost::diagnostics()`:

```cpp
const auto& diag = host.diagnostics();
fmt::print("Ticks: {}, Avg: {:.3f}ms\n", diag.tick_count, diag.average_tick_ms);

// Stage timings
for (const auto& stage : diag.stage_timings) {
    fmt::print("  {}: {:.3f}ms\n", stage.name, stage.last_ms);
}

// Streaming metrics
fmt::print("Streaming: {}/{} completed\n", 
    diag.streaming.streaming_total_completed,
    diag.streaming.streaming_total_requests);

// Scene validation
if (diag.scene_validation.has_cycles) {
    fmt::print("Scene has {} cycles\n", diag.scene_validation.cycle_count);
}
```

### Available Metrics

- **Lifecycle counters**: `initialize_count`, `tick_count`, `shutdown_count`, `initialize_failure_count`
- **Timing data**: `last_*_ms`, `max_*_ms`, `average_tick_ms` for each stage
- **Streaming telemetry**: Worker health, queue depth, completion/failure rates (see [`ASYNC_STREAMING_INTEGRATION.md`](ASYNC_STREAMING_INTEGRATION.md))
- **Scene validation**: Cycle detection, depth analysis, alert levels (see [`DIAGNOSTICS.md`](DIAGNOSTICS.md))
- **Physics collisions**: Contact manifolds, broad-phase metrics
- **Handle validation**: Asset and rendering handle lifecycle tracking (when `ENGINE_ENABLE_ASSETS` is on)

See [`DIAGNOSTICS.md`](DIAGNOSTICS.md) for complete metric reference and troubleshooting workflows.

## Async Streaming Integration

The runtime manages the IO thread pool for asynchronous asset loading:

```cpp
deps.streaming_config.worker_count = 2;
deps.streaming_config.queue_capacity = 64;
deps.streaming_config.enable = true;
```

Access streaming health metrics via `engine::runtime::streaming_metrics()` or through the diagnostics snapshot. See [`ASYNC_STREAMING_INTEGRATION.md`](ASYNC_STREAMING_INTEGRATION.md) for detailed integration patterns.

## Rendering Submission

When built with rendering support (`ENGINE_ENABLE_RENDERING=ON`), the runtime submits frame graphs:

```cpp
#if ENGINE_ENABLE_RENDERING
engine::runtime::RuntimeHost::RenderSubmissionContext context{
    .scheduler = my_scheduler,
    .backend = rendering::Backend::Vulkan
};
host.submit_render_graph(context);
#endif
```

Frame-graph metadata and resource events are captured in `diagnostics().frame_graph_serialization` and `diagnostics().frame_graph_events`.

## C API for Tooling

The runtime exposes a C API for Python bindings and external tooling:

```c
engine_runtime_initialize();
engine_runtime_tick(0.016);
size_t body_count = engine_runtime_body_count();
const auto& diag = engine::runtime::diagnostics();
engine_runtime_shutdown();
```

Python scripts in `scripts/diagnostics/` consume these APIs to generate telemetry reports and performance snapshots.

## Configuration

Key configuration surfaces:

- `RuntimeHostDependencies::scene_name`: Labels for diagnostics and telemetry
- `RuntimeHostDependencies::enabled_subsystems`: Explicit subsystem selection
- `RuntimeHostDependencies::streaming_config`: IO thread pool tuning
- Environment variable `ENGINE_PLATFORM_WINDOW_BACKEND`: Override window backend at runtime

## Dependencies

- **Core**: ECS registry, telemetry schema, plugin interfaces, IO thread pool
- **Animation**: Clip evaluation, blend trees, deformation transforms
- **Physics**: Rigid-body simulation, collision detection
- **Geometry**: Mesh deformation, spatial queries, bounds computation
- **Scene**: Hierarchy validation, transform propagation
- **Assets** (optional): Handle validation, async queue metrics
- **Rendering** (optional): Frame-graph submission, resource tracking
- **Compute**: Kernel dispatcher for physics and deformation workloads

## Testing

Integration tests live in `engine/tests/integration/test_runtime_integration.cpp` and cover:

- Lifecycle validation (initialize, tick, shutdown)
- Subsystem registration and dependency resolution
- Frame-graph submission determinism
- Streaming telemetry accuracy
- Scene validation integration

Run tests via:
```bash
ctest --preset clang-debug -R runtime
```

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module-specific task tracking
- [`DIAGNOSTICS.md`](DIAGNOSTICS.md): Comprehensive telemetry reference and troubleshooting
- [`async_streaming_integration.md`](async_streaming_integration.md): Asset loading workflows
- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md): System-level data flow and invariants
- [`../../design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md): Shared metric definitions
- [`../../tasks/T-0104-runtime-frame-graph-integration.md`](../../tasks/T-0104-runtime-frame-graph-integration.md): Frame-graph integration milestone

