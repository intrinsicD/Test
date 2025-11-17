# Geometry Viewer Implementation Summary

## Status: ✅ Rendering + Asset Streaming Through Runtime Application

The geometry viewer sample runs on top of `engine::runtime::Application` and exercises the full rendering stack: drag-and-dropped meshes and point clouds stream through the asset caches, are resolved by the OpenGL presentation backend, and render via the research baseline frame graph.

---

## Key Capabilities

| Area | Highlights |
|------|------------|
| **Runtime Integration** | Inherits from `engine::runtime::Application`, delegates window creation, input handling, and main-loop orchestration to the runtime host. |
| **Rendering Backend** | Configures the OpenGL presentation backend and compiles the research baseline frame graph (`engine/rendering/pipeline/research_baseline.hpp`). When GLFW/GLAD are missing the runtime falls back to a mock presentation mode so workflow smoke tests can still run headless. |
| **Asset Streaming** | Supports drag-and-drop for meshes (`.obj`, `.ply`, `.stl`) and point clouds (`.ply`, `.pcd`, `.xyz`). Assets load through `MeshCache` / `PointCloudCache` and resolve via backend-provided resource resolvers. |
| **Procedural Geometry** | Provides a procedural unit cube stored in an internal cache so the viewer renders content even before streaming external assets. |
| **Camera Controls** | Orbit camera driven by the unified input system (`engine::platform::input::InputState`). Mouse drag rotates, mouse wheel zooms, `Esc` quits. |
| **Scene Management** | Uses EnTT-based scene registry with render-geometry components. Bounds-based camera focusing centers new assets immediately after load. |
| **Diagnostics Panels** | Press `G` to toggle the Hybrid Workflow diagnostics menu wired through `engine::tools::editor::RuntimePanelBridge`, exposing runtime/asset/performance panels registered in the shared `PanelRegistry`.【F:engine/tools/examples/geometry_viewer.cpp†L717-L779】 |

---

## Source Layout

- Implementation: [`engine/tools/examples/geometry_viewer.cpp`](../../engine/tools/examples/geometry_viewer.cpp)
- CMake target: [`engine/tools/examples/CMakeLists.txt`](../../engine/tools/examples/CMakeLists.txt)

---

## Build Instructions

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target geometry_viewer
```

The viewer depends on platform windowing (GLFW) and OpenGL loaders. When these libraries are unavailable (e.g., inside the minimal CI container) the build preset will report the missing dependencies and skip the executable; document this limitation in your task brief. At runtime the application now logs that OpenGL is disabled and continues in headless mode instead of throwing an exception.【F:engine/tools/examples/geometry_viewer.cpp†L61-L109】

---

## Running the Viewer

```bash
./out/build/linux-gcc-debug/engine/tools/examples/geometry_viewer
```

> **Tip:** Launch the executable from a working directory that contains your assets so drag-and-drop resolves relative paths cleanly.  For headless environments use a virtual framebuffer (e.g., `xvfb-run`). When GLFW is unavailable the executable still runs, prints an explicit warning, and exercises input/asset pathways without attempting to render.【F:engine/tools/examples/geometry_viewer.cpp†L61-L109】

---

## Controls

| Input | Behaviour |
|-------|-----------|
| Left Mouse Drag | Orbit around the origin using spherical coordinates. |
| Mouse Scroll | Zoom in/out with clamped radius (1.0–50.0 units). |
| `T` | Toggle visibility of the procedural cube.【F:engine/tools/examples/geometry_viewer.cpp†L624-L676】 |
| `Delete` | Remove the most recently loaded model (LIFO).【F:engine/tools/examples/geometry_viewer.cpp†L630-L714】 |
| `G` | Show or hide the diagnostics/panel overlay registered through the hybrid workflow bridge.【F:engine/tools/examples/geometry_viewer.cpp†L636-L779】 |
| `Esc` | Exit the application. |

---

## Execution Flow Overview

1. **Runtime Setup** – `GeometryViewerApp` configures windowing, enables rendering, and supplies resolver lambdas so the backend can fetch meshes or point clouds from either the asset caches or the procedural store.
2. **Initialization** – On startup the app initializes the OpenGL backend, compiles the research baseline frame graph, seeds the procedural cube, and spawns camera/render entities inside the scene registry.
3. **Main Loop** – Each frame the runtime host pumps events, updates input, and calls `on_update`. The app processes drag-and-drop events, polls asset caches, updates the orbit camera, and logs FPS diagnostics.
4. **Streaming Pipeline** – When files are dropped, `engine::io::detect_geometry_file` selects the correct loader. Loaded assets focus the camera on their bounds and attach `RenderGeometry` components so the frame graph renders them immediately.

---

## Extensibility Notes

- Register additional procedural assets by storing meshes in `ProceduralMeshStorage` and adding corresponding validators.
- To experiment with different shading modes, adjust `ResearchBaselineOptions` in `setup_backend()` before compiling the frame graph.
- Integrate new asset formats by extending `engine::io::detect_geometry_file` and adding descriptors in the asset system.

The viewer serves as a canonical example of how runtime applications integrate rendering, asset streaming, and scene management without bypassing engine subsystems.
