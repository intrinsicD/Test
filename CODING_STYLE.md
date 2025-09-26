# Coding Style Guide

This document captures the conventions to follow when contributing C++ and Python code to the modular 3G rendering and geometry processing engine.

## General Principles

- Write self-documenting code and complement it with concise comments when behaviour is non-obvious.
- Keep public APIs minimal and focused. Prefer free functions in headers that forward to implementation files.
- Ensure every new feature includes build or usage instructions in the relevant documentation.

## C++ Guidelines

- Target **C++20** and avoid compiler extensions that are not part of the standard.
- Place declarations in headers under `engine/<module>/include` and implementations in `engine/<module>/src`.
- Expose C-compatible entry points for shared libraries using the `ENGINE_<MODULE>_EXPORTS` pattern so the modules remain callable from Python and other languages.
- Prefer `std::string_view`, `std::span`, and other non-owning views instead of raw pointers when sharing data.
- Use `#pragma once` include guards and keep headers free of unnecessary includes.
- Format code with four spaces per indentation level and brace-initialisation for aggregates.
- Keep functions short and focused; extract helpers into unnamed namespaces in the `.cpp` translation units when they are not part of the public API.
- When adding new libraries, update the corresponding `CMakeLists.txt` files to expose headers and shared exports consistently.

## Python Guidelines

- Target **Python 3.10+** and use modern typing features (PEP 604 unions, type annotations everywhere).
- Structure reusable logic into modules under `python/engine3g` and keep package exports curated in `__init__.py`.
- Prefer `pathlib.Path` over string path manipulation and use `ctypes` or `cffi` for FFI bindings as appropriate.
- Apply [PEP 8](https://peps.python.org/pep-0008/) naming conventions: `snake_case` for functions and variables, `PascalCase` for classes, and constants in `UPPER_CASE`.
- Document public functions and classes with docstrings describing intent, arguments, and return values.
- Avoid hard-coded paths; expose configuration via environment variables or explicit arguments so integrations remain flexible.

## Testing and Validation

- Ensure `cmake -S . -B build` followed by `cmake --build build` succeeds before submitting changes.
- For Python utilities, add usage examples or doctests when practical and keep imports side-effect free.
- Validate that new or modified modules export the expected C symbols to maintain interoperability guarantees.

