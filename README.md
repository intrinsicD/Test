# Test Engine Workspace

The `Test` repository hosts a modular real-time engine prototype. The tree is organised around a CMake build driven
core (`engine/`), tooling (`python/`, `scripts/`), and project documentation (`docs/`).

## Vision

- A flexible, high-performance engine architecture that can be extended and adapted for various real-time applications.
- A collaborative development environment that encourages contributions and experimentation.
- Comprehensive documentation to facilitate understanding and usage of the engine.
- Robust testing infrastructure to ensure code quality and reliability.
- Cross-platform support to reach a wide range of users and devices.
- Integration with modern development tools and workflows.
- A focus on modularity to allow developers to pick and choose components as needed.
- An open-source approach to foster community involvement and innovation.
- A commitment to continuous improvement and evolution of the engine based on user feedback and technological
  advancements.
- A user-friendly interface for both developers and end-users to interact with the engine.
- Support for a variety of asset types and formats to accommodate diverse project requirements.
- Scalability to handle projects of varying sizes and complexities.
- A strong emphasis on performance optimization to ensure smooth real-time experiences.
- Embrace the latest advancements in graphics, physics, and AI to keep the engine at the forefront of technology.
- Make full use of the ECS (Entity-Component-System) architecture to promote clean code and separation of concerns.
- Use DOD (Data-Oriented Design) principles to enhance performance and memory efficiency for components that are
  frequently updated or accessed.
- Provide a plugin system to allow third-party developers to extend the engine's capabilities without modifying the core
  codebase.
- Use a render graph and a task graph to manage rendering and processing tasks efficiently, allowing for better resource
  management and parallelism.
- Implement a scene graph to manage and organize the hierarchical structure of objects in a scene, facilitating
  transformations and rendering.
- Support hot-reloading of assets and code to enable rapid iteration and testing during development.
- Provide a comprehensive suite of tools for asset management, scene editing, and debugging to streamline the
  development workflow.
- Ensure compatibility with popular development environments and version control systems to facilitate collaboration
  among team members
- Make use of third-party: entt, imgui, spdlog.
- Use modern C++ (C++20 and beyond) to leverage the latest language features and best practices for performance and
  maintainability.

## Top-level Layout

- `docs/` – Design notes and API references that describe the evolving architecture.
- `engine/` – C++ engine source organised by subsystem (animation, rendering, physics, etc.).
- `python/` – Python bindings and companion utilities for automation and prototyping.
- `scripts/` – Developer tooling for builds and continuous integration jobs.
- `third_party/` – External dependencies vendored into the workspace (currently GoogleTest).
- `CMakeLists.txt` – Root CMake project file that wires the modular subprojects together.
- `CODING_STYLE.md` – Canonical formatting and style conventions for contributions.

## Build Requirements

The project is configured with CMake. To configure and build the workspace:

```bash
cmake -S . -B build
cmake --build build
```

Tests are provided via GoogleTest. Once the project is built, execute `ctest --test-dir build` to run the available
suites.

_Last updated: 2025-10-05_
