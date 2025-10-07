# Third-party Dependencies

External libraries vendored into the repository are tracked here. Each dependency provides its own README for usage details.

## Current Dependencies
- `entt/` – Fast and reliable Entity-Component-System (ECS) library that underpins the scene subsystem.
- `imgui/` – Bloat-free Immediate Mode Graphical User interface for C++ used by the tooling experiments.
- `spdlog/` – Fast C++ logging library shared by runtime and diagnostics layers.
- `googletest/` – Upstream GoogleTest source and headers for unit testing across C++ modules.

Each directory mirrors the upstream project layout and is consumed via add_subdirectory within the CMake build.

_Last updated: 2025-02-14_
