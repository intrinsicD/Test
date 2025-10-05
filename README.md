# Test Engine Workspace

The `Test` repository hosts a modular real-time engine prototype. The tree is organised around a CMake build driven core (`engine/`), tooling (`python/`, `scripts/`), and project documentation (`docs/`).

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

Tests are provided via GoogleTest. Once the project is built, execute `ctest --test-dir build` to run the available suites.

_Last updated: 2025-10-05_
