# I/O Module

## Current State

- Establishes the directory layout for importers, exporters, caching, and runtime I/O infrastructure.
- Provides initial stubs that will host asset streaming and persistence logic.

## Usage

- Implement specific format handlers under `importers/` and `exporters/` as pipelines solidify.
- Keep cache policies and runtime integration code in `src/` aligned with subsystem expectations.

## TODO / Next Steps

- Wire format import/export and caching to serve real content pipelines.
