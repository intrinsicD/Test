# Src

_Path: `engine/platform/src`_

_Last updated: 2025-10-06_

## Contents

### Files

- `api.cpp` – Exposes the module metadata entry points.
- `window_console.cpp` – Implements the reusable interactive window console.
- `window_system.cpp` – Builds the public window/event queue factory and routes backend selection.

### Directories

- `windowing/` – Concrete backend implementations shared by the window system (headless, GLFW, SDL stubs).

## Notes

- The GLFW backend pulls in `third_party/glfw` during configuration. Ensure your toolchain has access to the necessary windowing dependencies for your platform (X11/Wayland on Linux, Cocoa on macOS, Win32 on Windows).
