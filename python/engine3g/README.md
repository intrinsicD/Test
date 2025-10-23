# Python Engine Bindings

## Current State

- Contains the Python loader that discovers and interacts with compiled engine modules.
- Establishes the namespace for higher-level scripting utilities.

## Usage

- Ensure the engine shared libraries are discoverable before importing this package. The loader searches explicit overrides from
  `ENGINE3G_LIBRARY_PATH`, the package directory, the current working directory, and finally defers to the platform dynamic
  loader search path. Configure `ENGINE3G_LIBRARY_PATH` when libraries live outside those defaults.
- Explicit search paths passed to the loader can be provided as strings or `pathlib.Path` instances; the helper normalises them
  before probing candidates so custom directories work with either form.
- Module loaders accept either canonical identifiers (for example `engine_geometry`) or shorthand names (`geometry`,
  `rendering.core`). Both forms resolve to the same shared library prefix to simplify runtime integration scripts.
- `EngineRuntimeHandle.load_modules()` validates that each module name reported by the runtime is unique and aborts with a
  descriptive error if duplicates appear, preventing partially constructed module maps when subsystem registration is
  misconfigured.
- `EngineRuntimeHandle` implements the context manager protocol. Use `with loader.load_runtime() as runtime:` to automatically
  initialise the runtime on entry and shut it down on exit. When the runtime was already initialised, the context manager
  preserves the existing lifetime so shared handles remain valid.
- Add ergonomic wrappers or CLI entry points alongside new runtime capabilities.

## TODO / Next Steps

- Harden the loader API further by layering typed helpers over the runtime context manager and surfacing structured error
  handling.
