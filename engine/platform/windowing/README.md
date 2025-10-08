# Windowing

_Path: `engine/platform/windowing`_

_Last updated: 2025-10-06_

This namespace is implemented under `engine/platform/src/windowing/`.
It provides the concrete window backends used by the platform module:

- `window_base.*` – Shared infrastructure for in-memory event queues and the headless implementation.
- `mock_window.cpp` – Deterministic backend used for tests and server builds.
- `glfw_window.cpp` – Feature-complete GLFW integration that drives on-screen windows when the host supports it.
- `sdl_window.cpp` – Placeholder stub awaiting a future SDL integration.

Refer to the source files for implementation details and to extend the backend matrix.
