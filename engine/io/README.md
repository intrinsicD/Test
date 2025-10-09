# I/O Module

## Current State

- Establishes the directory layout for importers, exporters, caching, and runtime I/O infrastructure.
- Provides initial stubs that will host asset streaming and persistence logic.

## Usage

- Implement specific format handlers under `importers/` and `exporters/` as pipelines solidify.
- Keep cache policies and runtime integration code in `src/` aligned with subsystem expectations.
- Link against `engine_io`; consumers inherit `engine::project_options` and obtain the exported headers via `engine::headers`.
- Leverage the shared presets to configure and test (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

- Wire format import/export and caching to serve real content pipelines.
