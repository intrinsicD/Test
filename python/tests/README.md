# Python Loader Tests

## Current State

- Contains `test_loader.py`, a unittest suite validating the behaviour of `engine3g.loader`.
- Exercises canonical identifier generation, platform-specific shared library naming, search path aggregation, and error handling during dynamic loading.
- Uses lightweight stand-ins for C symbols to verify handle helpers without requiring compiled engine libraries, relying heavily on `unittest.mock` and dummy callable wrappers.

## Usage

- Ensure the repository root is on `PYTHONPATH` (the tests insert it automatically when run from this directory).
- Execute the suite with `pytest python/tests` or `python -m unittest python.tests.test_loader` after activating the virtual environment described in [`python/README.md`](../README.md).
- Set `ENGINE3G_LIBRARY_PATH` when you wish to exercise search path logic against real shared libraries compiled from the C++ build.

## TODO / Next Steps

- Expand coverage to load actual shared libraries produced by the C++ presets, using platform-aware fixtures instead of mocks.
- Add regression tests for error messages and logging once the loader surfaces structured diagnostics.
- Introduce reusable helpers for manipulating environment variables and temporary directories across future Python test modules.
