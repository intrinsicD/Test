# Engine Tools Profiling

## Current State

- Contains scaffolding files that will evolve alongside the subsystem.
- Depends on instrumentation emitted by runtime, rendering, compute, and platform layers.

## Objectives

- Provide live and offline profiling capabilities to diagnose performance regressions and guide optimization.
- Support frame timelines, job scheduler metrics, GPU counters, and asset pipeline analytics.

## Roadmap Alignment

- **Phase 0** – Share configuration, logging, and UI primitives from the tooling common library.
- **Phase 2** – Deliver capture, visualization, and benchmarking tooling integrated with CI.
- **Phase 4** – Package profilers as standalone binaries and documentation for distribution.

## Immediate Next Steps

1. Define instrumentation schema (zones, counters, async events) compatible with Chrome trace JSON.
2. Survey subsystems for existing profiling hooks and document required additions (CPU/GPU timers, counters).
3. Prototype capture pipeline writing trace files and streaming data to live viewers.
4. Design visualization panels (timelines, flame graphs, statistics) leveraging ImPlot or similar libraries.
5. Integrate profiling runs into performance test presets with thresholds and regression reporting.

## Dependencies & Open Questions

- Requires precise clock synchronization between CPU and GPU timelines.
- Needs sampling strategy for multithreaded workloads to avoid prohibitive overhead.
- Evaluate integration with external profilers (Tracy, RenderDoc) versus bespoke tooling.
