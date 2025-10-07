# Python Tooling

Python utilities complement the C++ engine. They facilitate automation, experimental scripting, and bindings.

## Layout
- `engine3g/` – Python package exposing shared-library loading helpers and future binding entry points.
- `tests/` – Automated tests for the Python-facing APIs (currently `test_loader.py`).

## Getting Started

Create a virtual environment targeting Python 3.12 (or later), install development requirements if needed, and run the
test suite:

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt  # when the package list becomes available
pytest
```

To experiment with the loader manually:

```python
from engine3g import loader

runtime = loader.load_runtime()
modules = runtime.load_modules()
print(runtime.name(), modules.keys())
```

Set the `ENGINE3G_LIBRARY_PATH` environment variable (colon-separated on POSIX, semicolon-separated on Windows) to point
at directories containing the compiled engine shared libraries when testing against native builds.

_Last updated: 2025-02-14_
