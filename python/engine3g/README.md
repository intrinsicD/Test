# Python Engine Bindings

## Current State

- Contains the Python loader that discovers and interacts with compiled engine modules.
- Establishes the namespace for higher-level scripting utilities.

## Usage

- Ensure `ENGINE3G_LIBRARY_PATH` points to the built shared libraries before importing this package.
- Add ergonomic wrappers or CLI entry points alongside new runtime capabilities.

## TODO / Next Steps

- Harden the loader API and expose ergonomic runtime bindings.
