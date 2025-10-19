# Python Tooling

## Current State

- Provides the root for Python-based automation, testing, and runtime bindings.
- Relies on the native modules to be discoverable via environment configuration.

## Usage

- Execute `pytest` after building native modules to validate the Python side.

## Environment Setup

Follow these steps to configure a clean workspace-local environment:

1. **Create a virtual environment** (Python 3.12+):

   ```bash
   cd /path/to/test-engine
   python3 -m venv .venv
   ```

2. **Activate the environment**:

   - Linux/macOS:

     ```bash
     source .venv/bin/activate
     ```

   - Windows PowerShell:

     ```powershell
     .venv\Scripts\Activate.ps1
     ```

3. **Upgrade tooling and install development dependencies**:

   ```bash
   python -m pip install --upgrade pip
   python -m pip install -r python/requirements.txt
   ```

   The `requirements.txt` manifest lists automation and testing dependencies
   tracked under the [PY-001 roadmap item](../docs/ROADMAP.md#py-001-core-bindings).
   The current set includes:

   - **`pytest`** — regression suite execution.
   - **`pybind11-stubgen`** — generates type stubs for compiled bindings so
     editor integrations and downstream packages surface accurate signatures.
   - **`build`/`wheel`** — produce distributable wheels for the Python
     packaging pipeline.
   - **`twine`** — upload and verification tooling for eventual package
     publication.

   Additional packages (formatters, linters) can be added to this manifest as
   they are introduced; keep this section updated alongside new automation
   helpers.

4. **Expose the repository to `PYTHONPATH` when running ad-hoc scripts**:

   The tests add the root automatically, but manual invocations should export:

   ```bash
   export PYTHONPATH="$(pwd)/python:$PYTHONPATH"
   ```

   On Windows PowerShell use:

   ```powershell
   $env:PYTHONPATH = "$(Get-Location)\python;" + $env:PYTHONPATH
   ```

5. **Run the test suite**:

   ```bash
   pytest python/tests
   ```

   Use `python -m unittest python.tests.test_loader` when `pytest` is
   unavailable.

## TODO / Next Steps

- Document the `pybind11-stubgen` invocation patterns once bindings land so the
  generated type hints are reproducible in CI.
- Capture packaging runbook steps (build, wheel validation, and `twine` usage)
  alongside the future publishing workflow for
  [PY-001](../docs/ROADMAP.md#py-001-core-bindings).
