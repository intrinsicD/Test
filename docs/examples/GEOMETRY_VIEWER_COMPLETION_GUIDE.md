# Geometry Viewer Completion Guide

## Purpose

The geometry viewer now ships as a complete runtime application that exercises the OpenGL research baseline. This guide explains how to validate the sample, what dependencies are required for full rendering, and how to interpret the new headless fallback when platform prerequisites are missing.

## 1. Verify Build Prerequisites

The executable depends on GLFW (window system) and GLAD (OpenGL loader). During CMake configuration ensure both targets are present:

```bash
cmake --preset linux-gcc-debug
```

Expected diagnostics:

- `OpenGL GLAD target detected (glad::gl_core)` → GL function pointers available.
- `Skipping geometry_viewer example (missing targets: glfw).` indicates GLFW could not be built because the container lacks X11/Xrandr headers. Install `libxrandr-dev` (and related X11 development packages) before re-configuring to enable the real windowed build.【e03c46†L1-L14】

If GLFW is absent, the CMake logic will skip the viewer target; the runtime now detects this case and runs in a headless mode instead of crashing at startup.

## 2. Runtime Behaviour

When both GLFW and GLAD are available the app:

1. Creates a GLFW window via `engine::runtime::Application`.
2. Configures the OpenGL presentation backend and compiles the research baseline frame graph.
3. Seeds a procedural cube, registers drag-and-drop loaders, and focuses the orbit camera on streamed assets.

Press `G` once the scene is running to toggle the Hybrid Workflow diagnostics menu (runtime diagnostics, profiler, hierarchy, asset browser, telemetry panels) that now render through the shared panel registry, confirming the ImGui surface is wired correctly.【F:engine/tools/examples/geometry_viewer.cpp†L571-L639】【F:engine/tools/examples/geometry_viewer.cpp†L717-L779】

If either dependency is missing, the viewer logs a warning and disables OpenGL while keeping the rest of the runtime (input, asset loading, diagnostics) alive so workflow tests can still execute.【F:engine/tools/examples/geometry_viewer.cpp†L61-L109】【F:engine/tools/examples/geometry_viewer.cpp†L265-L283】

## 3. Bringing Up Rendering Locally

Follow these steps on a workstation with GPU access:

1. Install the required system packages (`libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`, OpenGL headers).
2. Reconfigure the build preset and confirm CMake now creates the `geometry_viewer` executable.
3. Build and launch the viewer:

   ```bash
   cmake --build --preset linux-gcc-debug --target geometry_viewer
   ./out/build/linux-gcc-debug/geometry_viewer
   ```

4. Drag `.obj`, `.ply`, `.stl`, `.pcd`, or `.xyz` files into the window, orbit with left mouse drag, zoom with the scroll wheel, and exit with `Esc`.

## 4. Validating Headless Mode

In CI or other environments without GLFW, the viewer logs that rendering is disabled and exits cleanly after running its main loop without GPU work. Use this mode to smoke-test asset ingestion, event handling, and camera logic without GPU dependencies. Tests should assert that the warning appears so it is obvious when the build farm lacks the necessary system headers.【F:engine/tools/examples/geometry_viewer.cpp†L61-L109】

## 5. Troubleshooting Checklist

| Symptom | Likely Cause | Resolution |
|---------|--------------|------------|
| CMake skips `geometry_viewer` | `glfw` target absent (missing X11 headers) | Install `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`, rerun CMake. |
| Runtime prints “OpenGL presentation disabled” | GLAD or GLFW not configured | Verify third-party build, ensure `ENGINE_ENABLE_GLFW=ON`, rebuild. |
| Black window with assets loaded | Shaders or buffers failed to compile/upload | Check OpenGL logs (search for `OpenGL shader compilation failed`); run with validation layers enabled. |

Document configuration evidence (CMake output and runtime logs) in the task’s **Evidence** section whenever you validate the viewer.
