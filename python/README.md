# Python Tooling

Python utilities complement the C++ engine by providing shared-library discovery, automation hooks, and future scripting
entry points.

## Module Purpose

- `engine3g/` – Package that exposes the loader (`loader.py`) responsible for discovering `engine_runtime` and all
  `engine_<subsystem>` shared libraries.
- `tests/` – Pytest-based regression suite that exercises the loader and ensures Python-level APIs remain stable.

## Dependencies

- Python 3.12+
- `pytest`
- (Optional) `mypy` or similar static analysis tools when developing new bindings.

## Setup

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt  # populated as packages are introduced
pip install -e .[dev]  # optional extras when defined
```

Set `ENGINE3G_LIBRARY_PATH` (colon-separated on POSIX, semicolon-separated on Windows) to directories that contain the
compiled engine shared libraries so that `loader.load_runtime()` can succeed during local development.

## Build/Test Commands

- `pytest` – Run from the repository root or inside `python/` once the virtual environment is active.

## Usage Example

```python
from engine3g import loader

runtime = loader.load_runtime()
modules = runtime.load_modules()
print(runtime.name(), modules.keys())
```

_Last updated: 2025-02-14_
