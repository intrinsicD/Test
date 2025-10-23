# Core Module

## Overview

The core module provides foundational infrastructure used throughout the engine: an EnTT-backed ECS registry, plugin/subsystem discovery, telemetry and diagnostics schema, IO thread pool for async operations, and shared utility types.

## Entity-Component System (ECS)

### Registry

The core wraps EnTT's registry with engine-specific conveniences:

```cpp
#include "engine/core/ecs/registry.hpp"

auto& registry = core::Registry::instance();

// Create entities
auto entity = registry.create();

// Add components
registry.emplace<Position>(entity, 0.0f, 1.0f, 0.0f);
registry.emplace<Velocity>(entity, 1.0f, 0.0f, 0.0f);

// Query components
auto view = registry.view<Position, Velocity>();
for (auto [entity, pos, vel] : view.each()) {
    pos.x += vel.x * dt;
}

// Remove components
registry.remove<Velocity>(entity);

// Destroy entities
registry.destroy(entity);
```

### Systems

Systems process entities with specific component combinations:

```cpp
#include "engine/core/ecs/system.hpp"

class PhysicsSystem : public core::System {
public:
    void update(core::Registry& registry, float dt) override {
        auto view = registry.view<Position, Velocity>();
        for (auto [entity, pos, vel] : view.each()) {
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;
            pos.z += vel.z * dt;
        }
    }
};
```

## Plugin Architecture

### Subsystem Interface

Custom subsystems implement `ISubsystemInterface`:

```cpp
#include "engine/core/plugin/isubsystem_interface.hpp"

class MySubsystem : public core::plugin::ISubsystemInterface {
public:
    std::string_view name() const override { return "my_subsystem"; }
    
    std::vector<std::string> dependencies() const override {
        return {"core", "io"};  // Declare dependencies
    }
    
    void initialize() override {
        // Setup code
    }
    
    void tick(double dt) override {
        // Per-frame update
    }
    
    void shutdown() override {
        // Cleanup code
    }
};
```

### Subsystem Registry

The runtime manages subsystems through the registry:

```cpp
#include "engine/runtime/subsystem_registry.hpp"

auto registry = std::make_shared<runtime::SubsystemRegistry>();

// Register subsystem
registry->register_subsystem(std::make_shared<MySubsystem>());

// Initialize all (respects dependency order)
registry->initialize_all();

// Tick all subsystems
registry->tick_all(delta_time);

// Shutdown all (reverse dependency order)
registry->shutdown_all();
```

### Dependency Validation

The registry automatically detects cycles:

```cpp
// If subsystem A depends on B, and B depends on A:
// RuntimeError::dependency_cycle thrown with diagnostic message
```

Dependency cycle diagnostics include the full cycle path (e.g., "A → B → A") to aid troubleshooting.

## Telemetry & Diagnostics

### Metric Schema

The core defines the shared telemetry schema:

```cpp
#include "engine/core/telemetry/schema.hpp"

core::telemetry::MetricSet metrics;

// Counter
metrics.increment("runtime.lifecycle.initialize");

// Gauge
metrics.set_gauge("runtime.streaming.pending_tasks", pending_count);

// Histogram
metrics.record_duration("runtime.tick.animation_ms", duration_ms);

// Labels for attribution
metrics.increment("assets.load.failures", {{"format", "obj"}, {"reason", "parse_error"}});
```

### Metric Collection

Access metrics from any module:

```cpp
auto& diag = runtime::diagnostics();
const auto& metrics = diag.metrics;

// Query specific metrics
auto tick_count = metrics.get_counter("runtime.lifecycle.tick");
auto pending = metrics.get_gauge("runtime.streaming.pending_tasks");
```

### Structured Logging

The core integrates with spdlog for structured logging:

```cpp
#include "engine/core/diagnostics/logger.hpp"

core::log::info("Initialized subsystem: {}", subsystem_name);
core::log::warn("Cache eviction: asset={}, reason=ttl_expired", asset_id);
core::log::error("Validation failed: mesh={}, error={}", mesh_id, error_msg);
```

Logs include:
- Timestamps with microsecond precision
- Thread IDs for concurrency debugging
- Source location (file, line) in debug builds
- Structured key-value pairs for tooling integration

## Threading Infrastructure

### IO Thread Pool

Async operations use the shared IO thread pool:

```cpp
#include "engine/core/threading/io_thread_pool.hpp"

core::threading::IoThreadPoolConfig config{
    .worker_count = 4,
    .queue_capacity = 128,
    .enable = true
};

auto& pool = core::threading::IoThreadPool::instance();
pool.configure(config);
pool.start();

// Enqueue work
pool.enqueue([](){ 
    // Background task
    auto data = load_file("texture.png");
    return data;
});

// Shutdown
pool.stop();
```

The runtime automatically manages the thread pool lifecycle. Subsystems should use `IoThreadPool::instance()` rather than creating separate thread pools.

### Metrics

Thread pool telemetry includes:
- `worker_count`: Number of active workers
- `queue_capacity`: Maximum queued tasks
- `pending_tasks`: Current queue depth
- `total_enqueued`: Lifetime enqueue count
- `total_executed`: Lifetime execution count

## Configuration Management

### Application Config

```cpp
#include "engine/core/application/config.hpp"

core::application::Config config{
    .window_title = "My Application",
    .window_width = 1920,
    .window_height = 1080,
    .vsync = true
};
```

### Runtime Configuration

Environment variables override defaults:
- `ENGINE_PLATFORM_WINDOW_BACKEND`: Override window backend (glfw, sdl, mock)
- `ENGINE_LOG_LEVEL`: Set logging verbosity (trace, debug, info, warn, error)
- `ENGINE_TELEMETRY_OUTPUT`: Redirect telemetry to file

## Memory Utilities

### Arena Allocators

```cpp
#include "engine/core/memory/arena.hpp"

core::memory::Arena arena(1024 * 1024);  // 1MB
void* ptr = arena.allocate(256);
// No per-allocation free; reset entire arena
arena.reset();
```

### Object Pools

```cpp
#include "engine/core/memory/pool.hpp"

core::memory::Pool<MyObject> pool(1024);  // Pre-allocate 1024 objects
auto* obj = pool.acquire();
// Use object
pool.release(obj);
```

## Parallel Utilities

### Parallel For

```cpp
#include "engine/core/parallel/parallel_for.hpp"

std::vector<int> data(10000);
core::parallel::for_each(data, [](int& value) {
    value *= 2;
});
```

### Work Stealing Queue

```cpp
#include "engine/core/parallel/work_queue.hpp"

core::parallel::WorkQueue<Task> queue;
queue.push(task);
auto task = queue.try_pop();
```

## Error Handling

Following `DC-004`, core utilities use `Result<T, Error>`:

```cpp
#include "engine/core/result.hpp"

core::Result<int, std::string> compute() {
    if (error_condition) {
        return core::Error("computation failed");
    }
    return 42;
}

auto result = compute();
if (!result) {
    fmt::print("Error: {}\n", result.error());
} else {
    fmt::print("Success: {}\n", *result);
}
```

## Initialization Failure Diagnostics

The core tracks subsystem initialization failures:

```cpp
// When subsystem init fails, telemetry captures:
struct RuntimeInitializationFailure {
    std::string runtime;    // "RuntimeHost"
    std::string subsystem;  // "physics"
    std::string category;   // "dependency_missing"
    std::string message;    // Human-readable description
    double duration_ms;     // Time spent before failure
};

// Access via runtime diagnostics
const auto& diag = runtime::diagnostics();
if (diag.has_initialize_failure) {
    fmt::print("Last failure: {} in {}\n",
        diag.last_initialize_failure.message,
        diag.last_initialize_failure.subsystem);
}
```

Per-subsystem timing also tracks failures:
```cpp
for (const auto& subsystem : diag.subsystem_timings) {
    if (subsystem.initialize_failure_count > 0) {
        fmt::print("{}: {} failures, last: {}\n",
            subsystem.name,
            subsystem.initialize_failure_count,
            subsystem.last_initialize_failure_message);
    }
}
```

## Testing

Core tests validate:
- ECS registry operations (`test_registry.cpp`)
- Subsystem dependency resolution (`test_subsystem_registry.cpp`)
- Thread pool correctness (`test_io_thread_pool.cpp`)
- Telemetry accuracy (`test_telemetry.cpp`)
- Memory allocators (`test_memory.cpp`)

Run tests:
```bash
ctest --preset linux-gcc-debug -R core
```

## C API

Core exposes C bindings for scripting:

```c
extern "C" const char* engine_core_module_name();
```

Python bindings in `python/engine3g/` wrap the C API.

## Dependencies

- **EnTT**: ECS implementation
- **spdlog**: Structured logging
- **Dear ImGui**: Diagnostic UI (optional)

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones and completed work
- [`../../design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md): Comprehensive metric definitions
- [`../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md): How to add telemetry to new code
- [`../../design/PLUGIN_ARCHITECTURE.md`](../../design/PLUGIN_ARCHITECTURE.md): Subsystem plugin design
- [`../../design/ERROR_HANDLING_MIGRATION.md`](../../design/ERROR_HANDLING_MIGRATION.md): Result<T> usage patterns
- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md): System-level invariants and data flow

## Current State

- EnTT-backed registry façade, subsystem discovery helpers, module bootstrap plumbing, and dependency cycle diagnostics.
- Telemetry schema implemented with counters, gauges, and histograms; IO thread pool available for async tasks.

## Usage

- Run core tests:
  - `ctest --preset linux-gcc-debug -R core`
- See `engine/core/tests/` for ECS, telemetry, and threading examples.

## TODO / Next Steps

- Maintain runtime packaging automation in CI to validate telemetry tooling (`CR-135`); see ../../ROADMAP.md
- Extend static checks for legacy error patterns to guard `DC-004` compliance; see ../../ROADMAP.md
