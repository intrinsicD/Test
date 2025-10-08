# Engine3g Package

The `engine3g` package exposes Python bindings that locate and load the modular engine shared libraries.

## Module Purpose

- `loader.py` – Provides ctypes-based helpers to discover and load `engine_runtime` plus all `engine_<subsystem>` modules.
- `__init__.py` – Exports the loader utilities for convenient imports.

## Dependencies

- Python 3.12+
- Standard library modules (`ctypes`, `dataclasses`, `pathlib`) only; no third-party runtime dependencies are required.

## Setup

Follow the virtual environment instructions in [`python/README.md`](../README.md). Ensure `ENGINE3G_LIBRARY_PATH`
references the directories that contain the compiled shared libraries you intend to load.

## Usage

```python
from engine3g import loader

runtime = loader.load_runtime()
modules = runtime.load_modules()
print(runtime.name(), modules.keys())
```

If a shared library cannot be found, `EngineLibraryNotFound` will be raised with guidance on updating
`ENGINE3G_LIBRARY_PATH` or providing explicit search paths to the helper functions.

_Last updated: 2025-02-14_
