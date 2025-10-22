# Compute Module

## Overview

The compute module provides a unified kernel dispatcher for CPU and GPU compute workloads, with optional CUDA support. It manages kernel execution, dependency tracking, telemetry, and backend capability probing. The dispatcher is consumed by physics and geometry modules for parallel computations.

## Core Concepts

### Kernel Dispatcher

Execute compute kernels with automatic backend selection:

```cpp
#include "engine/compute/api.hpp"

compute::KernelDispatcher dispatcher;

// Register a kernel
dispatcher.register_kernel("particle_update", [](compute::KernelContext& ctx) {
    auto* positions = ctx.get_buffer<math::vec3>("positions");
    auto* velocities = ctx.get_buffer<math::vec3>("velocities");
    float dt = ctx.get_param<float>("delta_time");
    
    size_t count = ctx.get_buffer_size("positions");
    for (size_t i = 0; i < count; ++i) {
        positions[i] += velocities[i] * dt;
    }
});

// Dispatch kernel
compute::KernelParams params;
params.set_buffer("positions", position_data, count);
params.set_buffer("velocities", velocity_data, count);
params.set_param("delta_time", 0.016f);

auto result = dispatcher.dispatch("particle_update", params);
if (result) {
    fmt::print("Kernel executed in {:.3f}ms\n", result->duration_ms);
}
```

### Backend Abstraction

The dispatcher abstracts over multiple backends:

- **CPU**: Single-threaded or multi-threaded execution
- **CUDA**: GPU execution when `ENGINE_ENABLE_CUDA=ON`
- **Compute Shaders** (planned): Vulkan/DirectX compute pipelines

```cpp
// Query available backends
auto backends = compute::available_backends();
for (auto backend : backends) {
    fmt::print("Backend: {}\n", compute::backend_name(backend));
}

// Force specific backend
dispatcher.set_backend(compute::Backend::CUDA);
```

### CUDA Support

Enable CUDA kernels when built with CUDA support:

```cpp
#if ENGINE_ENABLE_CUDA
#include "engine/compute/cuda/api.hpp"

// Register CUDA kernel
dispatcher.register_cuda_kernel("cuda_transform", cuda_kernel_function);

// CUDA-specific configuration
compute::CudaConfig cuda_config{
    .device_id = 0,
    .streams = 4,
    .enable_peer_access = true
};
compute::cuda::configure(cuda_config);
#endif
```

### Execution Reports

Track kernel execution through detailed reports:

```cpp
compute::ExecutionReport report = dispatcher.execute_all();

fmt::print("Total kernels: {}\n", report.kernel_count);
fmt::print("Total duration: {:.3f}ms\n", report.total_duration_ms);

for (const auto& entry : report.entries) {
    fmt::print("  {}: {:.3f}ms on {}\n",
        entry.kernel_name,
        entry.duration_ms,
        compute::backend_name(entry.backend));
}
```

Reports are integrated with runtime diagnostics:

```cpp
const auto& runtime_state = runtime_host.tick(dt);
const auto& dispatch_report = runtime_state.dispatch_report;
// Access kernel timing information
```

## Dependency Tracking

Kernels can declare dependencies for automatic ordering:

```cpp
compute::KernelDescriptor desc{
    .name = "skinning",
    .dependencies = {"animation_eval", "joint_transform_compute"}
};

dispatcher.register_kernel(desc, kernel_function);

// Dispatcher automatically orders execution
dispatcher.dispatch_all();  // Respects dependency graph
```

### Cycle Detection

The dispatcher validates dependency graphs:

```cpp
// If kernel A depends on B, and B depends on A:
// ComputeError::dependency_cycle thrown
```

Cycle detection tooling is documented in the architecture improvement plan (`DC-001`).

## Backend Capabilities

Query backend capabilities before dispatching:

```cpp
auto caps = compute::query_capabilities(compute::Backend::CUDA);

if (caps.supports_double_precision) {
    // Use double precision
}

if (caps.max_threads_per_block >= 1024) {
    // Use larger thread blocks
}

fmt::print("Memory: {}MB\n", caps.total_memory_mb);
fmt::print("Compute capability: {}.{}\n", caps.major, caps.minor);
```

## Math Helpers

The compute module provides optimized helpers for common operations:

```cpp
#include "engine/compute/math_helpers.hpp"

// Identity transform (no-op for testing)
math::mat4 identity = compute::identity_transform();

// Parallel matrix multiplication
std::vector<math::mat4> results = compute::parallel_matmul(
    matrices_a, matrices_b, thread_count);

// Reduction operations
float sum = compute::reduce_sum(values);
float max_val = compute::reduce_max(values);
```

## Integration with Physics

The physics module uses the dispatcher for simulation:

```cpp
// In PhysicsWorld::step()
compute::KernelParams params;
params.set_buffer("positions", body_positions, body_count);
params.set_buffer("velocities", body_velocities, body_count);
params.set_param("dt", delta_time);
params.set_param("gravity", gravity_vector);

dispatcher.dispatch("integrate_bodies", params);
```

## Integration with Geometry

Geometry deformation uses compute kernels:

```cpp
// Linear blend skinning on GPU
compute::KernelParams params;
params.set_buffer("vertex_positions", mesh.positions, vertex_count);
params.set_buffer("joint_transforms", transforms, joint_count);
params.set_buffer("joint_weights", weights, vertex_count);

dispatcher.dispatch("linear_blend_skinning", params);
```

## Telemetry

Compute operations emit per-kernel telemetry:

```cpp
const auto& diag = runtime::diagnostics();
for (const auto& metric : diag.metrics.histograms) {
    if (metric.name.starts_with("compute.kernel.")) {
        fmt::print("Kernel {}: avg={:.3f}ms, max={:.3f}ms\n",
            metric.name, metric.average, metric.max);
    }
}
```

Metrics include:
- Per-kernel execution duration
- Backend utilization
- Memory transfer overhead (GPU backends)
- Dependency graph depth
- Dispatch queue depth

## Performance Considerations

Typical kernel dispatch overhead:
- CPU: ~0.01ms
- CUDA: ~0.1ms (includes kernel launch)
- Compute shader: ~0.2ms (includes pipeline bind)

Optimization tips:
- Batch similar kernels to reduce dispatch overhead
- Use dependency tracking instead of manual synchronization
- Profile with `ExecutionReport` to identify bottlenecks
- Prefer larger work groups on GPU backends

## Testing

Compute tests validate:
- Kernel registration and dispatch (`test_dispatcher.cpp`)
- Dependency graph validation (`test_dependencies.cpp`)
- Backend capability queries (`test_capabilities.cpp`)
- CUDA integration when enabled (`cuda/tests/test_cuda.cpp`)
- Execution report accuracy (`test_telemetry.cpp`)

Run tests:
```bash
ctest --preset clang-debug -R compute
```

CUDA tests (when enabled):
```bash
ctest --preset clang-debug -R compute_cuda
```

## Dependencies

- **Math**: Vector, matrix types for kernel parameters
- **Core**: Telemetry schema, error handling
- **CUDA Toolkit** (optional): Required when `ENGINE_ENABLE_CUDA=ON`

## Related Documentation

- [`ROADMAP.md`](ROADMAP.md): Module milestones including GPU sampling prototypes
- [`../../architecture.md`](../../architecture.md): Compute integration in simulation pipeline
- [`../../design/animation_gpu_parallel_sampling_benchmark.md`](../../design/animation_gpu_parallel_sampling_benchmark.md): Planned GPU animation sampling
- Used by: Physics module (body integration), Geometry module (deformation), Animation module (planned)


