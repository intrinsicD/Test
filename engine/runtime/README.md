# Runtime

_Path: `engine/runtime`_

_Last updated: 2025-03-15_


## Overview

The runtime library stitches subsystems together and exposes a stable C API for dynamic loading. It now maintains a
lightweight simulation loop that drives animation, compute scheduling, physics integration, and mesh deformation in
lockstep. The exported helpers in [`include/engine/runtime/api.hpp`](include/engine/runtime/api.hpp) provide:

- **`runtime_frame_state tick(double dt)`** – Step the simulation and return pose/bounds/body summaries.
- **`current_mesh()`** – Access the shared `SurfaceMesh` after deformation.
- **C bindings (`engine_runtime_*`)** – Invoke initialization, ticking, and state queries from Python or other hosts.

The GoogleTest suite verifies both module enumeration and that the execution graph runs the expected kernels.

## TODO

- Surface profiling counters (accumulated dt, kernel timings) through the runtime API for regression dashboards.
- Thread the ECS scheduler from `engine/core` into the runtime loop once gameplay systems arrive.
- Allow the runtime to load clips/meshes from disk instead of relying on the procedural defaults.
