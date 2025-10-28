# Python Tooling

## Current State

- Provides the root for Python-based automation, testing, and runtime bindings.
- Relies on the native modules to be discoverable via environment configuration.
- Ships a manually curated `.pyi` stub for `engine3g.loader` so editors and
  static analyzers have type information while `PY-001` bindings mature (see
  [docs/ROADMAP.md](../docs/ROADMAP.md#py-001-core-bindings)).

## Usage

- Execute `pytest` after building native modules to validate the Python side.
- Provide explicit search directories as strings or `pathlib.Path` objects when invoking `engine3g.loader` helpers; the loader
  normalises both forms while resolving shared libraries.
- The loader enforces unique module names when aggregating runtime subsystems, raising a descriptive error if duplicates are
  reported so misconfigured registries do not produce partially loaded module sets.
- `engine3g.config_schema.load_configuration()` and `load_dataset_manifest()` validate AI-004 configuration manifests
  (YAML or JSON) and surface `ConfigurationSchemaError` diagnostics when schema requirements are violated.
- `python -m scripts.validate_ai004_config --dataset manifest.json --config config.yaml` offers a command-line wrapper around
  the schema validators for CI pipelines and ad-hoc checks during AI-004 development.
- `python -m scripts.prototyping.run_prototype_harness --config <path> [--dry-run] [--require-schema]` validates and exercises
  the AI-004 prototyping harness scaffold. Use `--dry-run` to skip native runtime loading while still confirming configuration
  integrity. Pass `--require-schema` (or export `ENGINE_AI004_SCHEMA_V1=1`) to fail fast when manifests omit `ai-004.*`
  headers during the migration window. Provide `--describe-json <path>` to export dataset/rendering/runtime metadata for the
  TL-210 sandbox UI and `--summary-json <path>` to capture execution results for benchmark automation. Use
  `--case-study <id>` to launch packaged manifests from `assets/datasets/case_studies/` (for example,
  `geometry-baseline` or `rendering-debug`) without copying paths by hand. The repository ships a ready-to-run manifest at
  `docs/examples/ai004_sample.json` alongside the case study presets for quick smoke tests.
- Set the environment variable `ENGINE_AI004_SCHEMA_V1=1` to require schema headers in manifests. When unset, the loaders
  tolerate legacy manifests by injecting default headers so existing workflows remain functional during the migration window.
- Manage runtime lifetime ergonomically using the context manager exposed by `engine3g.loader.load_runtime()` or
  `EngineRuntimeHandle`; entering the context calls `initialize()` and exiting always calls `shutdown()` when the runtime was
  activated by the context:

  ```python
  from engine3g import loader

  with loader.load_runtime() as runtime:
      runtime.tick(1.0 / 60.0)
  ```

## Environment Setup

Run the automated bootstrap script to provision a workspace-local virtual
environment and install the shared dependencies:

```bash
python -m scripts.bootstrap_python_env
```

The script creates `.venv` by default, upgrades `pip`, installs
`python/requirements.txt`, and prints activation instructions for POSIX shells
and PowerShell. Use `--help` for additional options such as
`--venv-path <path>` or `--skip-install` when provisioning CI caches.

Manual setup instructions remain below for reference or for contributors who
prefer to manage the environment directly:

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

## Generating Type Stubs

`pybind11-stubgen` produces `.pyi` interfaces for the compiled bindings once
the native modules are built. Generate the stubs whenever the C++ bindings
change so editor integrations and downstream packages stay in sync.

1. **Build the native presets** that export the Python bindings. For example:

   ```bash
   cmake --preset linux-gcc-debug
   cmake --build --preset linux-gcc-debug --target <binding-target>
   ```

   Replace `<binding-target>` with the module-specific CMake target documented
   by the subsystem you are packaging (for example, consult the module README
   for names such as `engine_geometry_python`). The build directory produced by
   the preset will host the compiled `.so`/`.pyd` artefacts.

2. **Activate the virtual environment and install requirements** as described
   above. The stub generator ships via `pybind11-stubgen` in
   `python/requirements.txt`.

3. **Expose the build tree and package sources on `PYTHONPATH`** so the module
   is importable by the stub generator. When using the default presets this is
   typically:

   ```bash
   export PYTHONPATH="$(pwd)/python:$(pwd)/out/build/linux-gcc-debug/python"
   ```

   Adjust the build directory to match the preset you compiled.

4. **Invoke `pybind11-stubgen`** for each binding module. The command below
   emits stubs into `python/stubs/` and skips generating an auxiliary
   `setup.py`:

   ```bash
   pybind11-stubgen engine3g.runtime --output-dir python/stubs --root-suffix "" --no-setup-py
   ```

   Swap `engine3g.runtime` for the fully-qualified module you wish to
   document. Repeat the invocation for additional bindings (for example,
   `engine3g.geometry`).

5. **Validate and commit the stubs**. Ensure they import cleanly and regenerate
   them whenever the native signatures change:

   ```bash
   python -m compileall python/stubs
   ```

## Packaging Runbook

Use the Python packaging toolchain to produce wheels and source archives for
distribution. These steps assume you have already built the native modules for
the target platform.

1. **Clean previous artefacts** to avoid publishing stale builds:

   ```bash
   rm -rf dist build
   ```

2. **Build the package** using `python -m build`. This command produces both a
   wheel and an sdist under `dist/`:

   ```bash
   python -m build --wheel --sdist
   ```

3. **Validate metadata** before uploading. `twine check` verifies the wheel and
   sdist structure:

   ```bash
   python -m twine check dist/*
   ```

4. **Upload to the chosen repository**. Use TestPyPI while iterating; swap the
   repository once ready for production distribution:

   ```bash
   python -m twine upload --repository testpypi dist/*
   ```

   Provide credentials via environment variables or a `.pypirc` configuration.

5. **Record published versions** in release notes and tag the repository. Keep
   the roadmap (`docs/ROADMAP.md`) in sync when packaging milestones complete.

## TODO / Next Steps

- Automate stub generation and package verification in CI once bindings
  stabilise.
