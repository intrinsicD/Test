# Third-party Dependencies

External libraries vendored into the repository are tracked here. Each dependency provides its own *.cmake for either using a locally vendored copy or fetching it on-demand.

## Current Dependencies
- `cmake/entt.cmake` – Fast and reliable Entity-Component-System (ECS) library that underpins the scene subsystem.
- `cmake/glad.cmake` – Multi-language OpenGL loader generator used to load OpenGL functions at runtime.
- `cmake/glfw.cmake` – Window and context management library fetched at configure time to power the GLFW platform backend.
- `cmake/googletest.cmake` – Upstream GoogleTest source and headers for unit testing across C++ modules.
- `cmake/imgui.cmake` – Bloat-free Immediate Mode Graphical User interface for C++ used by the tooling experiments.
- `cmake/spdlog.cmake` – Fast C++ logging library shared by runtime and diagnostics layers.
- `cmake/vma.cmake` – Vulkan Memory Allocator to simplify and optimize memory management for Vulkan applications.
- `cmake/yaml-cpp.cmake` – YAML 1.2 parser/emitter fetched via CMake to validate AI-004 configuration manifests.

Each directory mirrors the upstream project layout and is consumed via `add_subdirectory` within the CMake build.
If a dependency submodule is absent locally (e.g., after a shallow clone), or present but missing its source
files (such as an empty checkout directory), CMake will fetch the required sources on-demand using
`FetchContent` during configuration.

_Last updated: 2025-10-06_
