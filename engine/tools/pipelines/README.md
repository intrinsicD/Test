# Engine Tools Pipelines

## Current State

- Contains scaffolding files that will evolve alongside the subsystem.
- Relies on `engine/io` importers/exporters and `engine/assets` schemas for asset definitions.

## Objectives

- Automate asset ingestion, transformation, and packaging with reproducible, incremental builds.
- Surface command-line and scripted workflows that integrate with CI and editor tooling.

## Roadmap Alignment

- **Phase 0** – Share configuration and logging utilities through the tooling common library.
- **Phase 1** – Implement the first content pipeline orchestrator and cache management.
- **Phase 4** – Deliver distributable command-line tools and project templates.

## Immediate Next Steps

1. Catalogue supported asset types and define required import/cook/output stages.
2. Design dependency graph model (nodes, edges, hashing strategy) to drive incremental builds.
3. Prototype a CLI that accepts project manifests and executes pipelines with progress reporting.
4. Integrate with filesystem abstraction for sandboxed read/write and virtualized asset roots.
5. Specify telemetry emitted during pipeline runs (timings, cache hits/misses) for profiling integration.

## Dependencies & Open Questions

- Needs deterministic hashing and versioning of source assets to guarantee reproducibility.
- Requires policy for third-party tool invocation (e.g., mesh optimizers) and sandboxing.
- Explore remote execution/distributed build support for large asset batches.
