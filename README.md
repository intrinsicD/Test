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

- `docs/` – Design notes, API references, and validation utilities that describe the evolving architecture.
- `engine/` – C++ engine source organised by subsystem (animation, rendering, physics, etc.).
- `python/` – Python bindings and companion utilities for automation and prototyping.
- `scripts/` – Developer tooling for builds, continuous integration jobs, and documentation validation.
- `third_party/` – External dependencies vendored into the workspace (EnTT, Dear ImGui, spdlog, GoogleTest).
- `CMakeLists.txt` – Root CMake project file that wires the modular subprojects together.
- `CODING_STYLE.md` – Canonical formatting and style conventions for contributions.

## Build Requirements

The project is configured with CMake. To configure and build the workspace:

```bash
cmake -S . -B build
cmake --build build
```

## Testing

- **C++** – After building with CMake, run `ctest --test-dir build` to execute the GoogleTest suites that ship with the
  engine modules (e.g., math numerics, geometry kernels).
- **Python** – Run `pytest` from the repository root to exercise the `engine3g.loader` helpers and ensure shared library
  discovery behaviour remains stable.

## Tooling and Dependencies

- Third-party libraries are vendored under `third_party/` and currently include [EnTT](https://github.com/skypjack/entt),
  [Dear ImGui](https://github.com/ocornut/imgui), [spdlog](https://github.com/gabime/spdlog), and
  [GoogleTest](https://github.com/google/googletest).
- Documentation links can be validated offline with `python scripts/validate_docs.py`.

## TODOs:
Here’s a clean split. I’ve grouped closely related items and kept your original wording.

# Basic Features

* Basic application framework with event loop and lifecycle management
* Configuration management (JSON, YAML)
* Logging (spdlog) and tracing (tracy)
* Cross-platform build configurations (Windows, macOS, Linux)
* Window management and multi-window support
* User input, mouse/keyboard/gamepad handling
* Entity-Component-System (ECS) architecture (EnTT)
* Data-Oriented Design (DOD) principles for performance-critical components
* Rendering backends (Vulkan, OpenGL, DirectX)
* Model loading (OBJ, FBX, glTF)
* Texture loading and management (DDS, KTX, PNG, JPEG)
* Scene graph for hierarchical object management
* Benchmarking and performance measurement tools
* Performance profiling and optimization tools

# Advanced Features

* Material system with PBR support and shader management
* Forward rendering pipeline
* Deferred rendering pipeline
* Hybrid rendering techniques (forward+deferred)
* Support for multiple light types (directional, point, spot, area)
* Shadow mapping techniques (CSM, PCF, VSM)
* Post-processing effects (bloom, tone mapping, anti-aliasing)
* Environment mapping (skyboxes, IBL)
* Global illumination (SSAO, IBL)
* HDR image support (EXR, HDR)
* Mouse picking and object selection (entity, triangle, edge, vertex)
* Frame graph for efficient rendering task scheduling
* Spatial partitioning structures (BVH, Octree, KDTree, Grid)
* Frustum culling and occlusion culling
* Level-of-detail (LOD) management for meshes and textures
* Texture streaming and memory management
* Scalar field rendering (volume rendering, isosurfaces)
  * Colormaps generation and application
  * Isolines
* Vectorfield rendering
* Particle systems and GPU-based simulations
* Physics engine with collision detection and rigid body dynamics
* Animation system with rigging, skinning, and keyframe interpolation
* Audio system for 3D sound and music playback
* UI framework for in-game menus and HUDs
* Scripting support (e.g., Lua, Python) for gameplay logic and rapid prototyping
* Asset import/export pipelines and caching layers
* Hot-reloading of assets and code modules
* Plugin system for modular extensions
* Job system for parallel task execution
* cuda support and rendering interop

# High-Level Features

* Networking layer for multiplayer and online features
* Comprehensive examples and sample projects to demonstrate engine capabilities
* Documentation generation and hosting (e.g., Doxygen, Sphinx)
* Continuous integration setup for automated builds and tests (e.g., GitHub Actions, Travis CI)
* Packaging and distribution mechanisms (e.g., installers, Docker images)
* Community engagement strategies (forums, Discord, GitHub discussions)
* Localization and internationalization support
* Accessibility features to ensure usability for all users

_Last updated: 2025-02-14_
