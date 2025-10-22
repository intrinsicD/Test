# Python Engine Bindings

## Current State

- Contains the Python loader that discovers and interacts with compiled engine modules.
- Establishes the namespace for higher-level scripting utilities.

## Usage

- Ensure the engine shared libraries are discoverable before importing this package. The loader searches explicit overrides from
  `ENGINE3G_LIBRARY_PATH`, the package directory, the current working directory, and finally defers to the platform dynamic
  loader search path. Configure `ENGINE3G_LIBRARY_PATH` when libraries live outside those defaults.
- Add ergonomic wrappers or CLI entry points alongside new runtime capabilities.

## TODO / Next Steps

- Harden the loader API and expose ergonomic runtime bindings.
