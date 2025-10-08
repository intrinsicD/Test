# Third-party Dependencies

External libraries vendored into the repository are tracked here. Each dependency provides its own README for usage details.

## Current Dependencies
- `entt/` – Fast and reliable Entity-Component-System (ECS) library that underpins the scene subsystem.
- `glfw/` – Window and context management library fetched at configure time to power the GLFW platform backend.
- `imgui/` – Bloat-free Immediate Mode Graphical User interface for C++ used by the tooling experiments.
- `spdlog/` – Fast C++ logging library shared by runtime and diagnostics layers.
- `googletest/` – Upstream GoogleTest source and headers for unit testing across C++ modules.

Each directory mirrors the upstream project layout and is consumed via add_subdirectory within the CMake build.
If a dependency submodule is absent locally (e.g., after a shallow clone), CMake will fetch the required sources
on-demand using `FetchContent` during configuration.

_Last updated: 2025-10-06_
