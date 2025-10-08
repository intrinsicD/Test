# Test Engine Workspace

The `Test` repository hosts a modular real-time engine prototype that couples modern C++ modules with supporting Python
tooling. This README provides a concise map of the workspace and the commands required to configure, build, and test it.

## Repository Layout

| Path | Purpose |
| --- | --- |
| `docs/` | Design notes, API references, and validation utilities for written documentation. |
| `engine/` | C++ source code organised by subsystem (animation, rendering, physics, etc.). |
| `python/` | Python bindings and automation helpers that load the compiled engine modules. |
| `scripts/` | Developer tooling for local builds, CI entry points, and documentation checks. |
| `third_party/` | Vendored dependencies such as EnTT, Dear ImGui, spdlog, and GoogleTest. |
| `CMakeLists.txt` | Root CMake project that wires the modular subprojects together. |
| `CODING_STYLE.md` | Canonical formatting and style conventions for contributions. |

## Prerequisites

- **Compilers** – A C++20-capable toolchain (MSVC 19.3x, Clang 15+, or GCC 12+).
- **Build system** – CMake 3.26 or newer and Ninja or the generator of your choice.
- **Python** – Python 3.12+ with `pip` to execute utility scripts and Python-based tests.
- **Host libraries** – Platform-native SDKs/drivers for the rendering backends you plan to compile (Vulkan SDK,
  DirectX 12 Agility SDK, or system OpenGL drivers).
  - Linux builds that enable the GLFW backend require the X11 development headers. Install `libxrandr-dev`,
    `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev` via your package manager before configuring CMake.

## Setup and Build

1. Configure a build directory with CMake.
2. Compile the default targets.

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Subsystems produce shared libraries named `engine_<subsystem>`; the runtime module aggregates them as
`engine_runtime`.

## Testing

- **C++ suites** – `ctest --test-dir build` executes the GoogleTest-based unit and integration tests shipped under
  `engine/tests/`.
- **Python bindings** – Activate your virtual environment and run `pytest` from the repository root to validate the
  loader located in `python/engine3g/loader.py`.
- **Documentation checks** – `python scripts/validate_docs.py` ensures Markdown links remain valid.

## Python Tooling Quickstart

Python helpers live under `python/` and expect the engine shared libraries to be discoverable. Set the
`ENGINE3G_LIBRARY_PATH` environment variable (colon-separated on POSIX, semicolon-separated on Windows) so the loader can
resolve `engine_runtime` and any `engine_<subsystem>` shared libraries you built locally.

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt  # populated as the tooling matures
pytest
```

## Maintenance Guidelines

- Update or create Markdown pages alongside any behavioural change.
- Run the available unit tests and documentation validation script before submitting pull requests.
- Prefer data-oriented, modular designs that match the repository layout; see `docs/design/` for current proposals.

## TODO

- **Implement core subsystem features** – The animation, geometry, physics, and compute modules currently expose only
  their `module_name()` exports and lack rigging, simulation, or job infrastructure. Flesh these subsystems out so that
  the runtime can exercise real behaviour.【F:engine/animation/src/api.cpp†L1-L13】【F:engine/geometry/src/api.cpp†L1-L13】【F:engine/physics/src/api.cpp†L1-L13】【F:engine/compute/src/api.cpp†L1-L16】【F:engine/compute/cuda/src/api.cpp†L1-L22】
- **Replace stubbed window backends** – The GLFW and SDL window factories simply wrap the headless mock window. Wire
  them up to real platform APIs and swapchain creation so rendering can target on-screen surfaces.【F:engine/platform/src/windowing/glfw_window.cpp†L1-L12】【F:engine/platform/src/windowing/sdl_window.cpp†L1-L12】
- **Provide a concrete RenderResourceProvider** – Rendering passes depend on a resource provider interface, but no
  implementation exists to upload meshes, materials, or shaders to the GPU. Implement a platform-backed provider and
  connect it to the frame-graph execution path.【F:engine/rendering/include/engine/rendering/render_pass.hpp†L22-L61】【4045be†L1-L2】
- **Reconcile build and tooling documentation** – The README asks for CMake 3.26 and a `requirements.txt`, yet the build
  targets CMake 3.20 and no requirements file is checked in. Update the build scripts and documentation or add the missing
  dependency manifest.【F:README.md†L21-L57】【F:CMakeLists.txt†L1-L38】【0f1a77†L1-L2】【a43702†L1-L2】
- **Expand subsystem tests** – The animation and physics suites only assert the exported module name. Add behavioural
  coverage once real features land so regressions surface in CI.【F:engine/animation/tests/test_module.cpp†L1-L8】【F:engine/physics/tests/test_module.cpp†L1-L8】

_Last updated: 2025-10-08_
