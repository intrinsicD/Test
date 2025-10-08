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

_Last updated: 2025-02-14_
