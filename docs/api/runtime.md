# Runtime Module

The runtime module aggregates subsystem entry points and exposes a stable C ABI for dynamic discovery. Its public
surface lives in [`engine/runtime/api.hpp`](../../engine/runtime/include/engine/runtime/api.hpp).

## Simulation loop

- `initialize()` / `shutdown()` – Manage lifetime of the shared simulation state.
- `runtime_frame_state tick(double dt)` – Steps animation, compute, physics, and geometry in a deterministic order and
  returns the resulting pose, body positions, bounds, and compute execution report.
- `const geometry::SurfaceMesh& current_mesh()` – Provides direct access to the deformed mesh for inspection.

The returned `runtime_frame_state::dispatch_report.execution_order` mirrors the kernel ordering produced by
`compute::KernelDispatcher`, making the integration easy to validate in tests.

## C bindings

The runtime exports a family of `engine_runtime_*` functions that mirror the C++ helpers and allow scripting layers to
coordinate the simulation. Highlights include:

- `engine_runtime_initialize()` / `engine_runtime_shutdown()`
- `engine_runtime_tick(double dt)`
- `engine_runtime_body_count()` + `engine_runtime_body_position(index, float[3])`
- `engine_runtime_joint_count()` + joint name/translation accessors
- `engine_runtime_mesh_bounds(float[3], float[3])`
- `engine_runtime_dispatch_count()` / `_dispatch_name()` to inspect kernel ordering

The Python bindings in `python/engine3g/loader.py` wrap these functions so end-to-end validation can be performed from
scripting environments.

_Last updated: 2025-03-15_
