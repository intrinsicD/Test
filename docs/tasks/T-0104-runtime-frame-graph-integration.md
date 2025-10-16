# T-0104: Runtime Frame-Graph Integration

## Goal
Expose a stable runtime API for scheduling rendering work and validate the end-to-end path through the Vulkan prototype.

## Inputs
- Code: `engine/runtime/runtime_host.cpp`, `engine/rendering/frame_graph.hpp`, `engine/rendering/tests/`
- Specs: [`docs/specs/ADR-0003-runtime-frame-graph.md`](../specs/ADR-0003-runtime-frame-graph.md)
- Roadmap: [`AI-003`](../specs/ADR-0003-runtime-frame-graph.md#motivation), [`RT-003`](../tasks/2025-02-17-sprint-06.md)

## Constraints
- Maintain deterministic ordering when multiple subsystems publish passes.
- Avoid introducing backend-specific logic into runtime; rely on scheduler interfaces.
- Preserve existing regression tests and add new ones instead of modifying expectations silently.

## Deliverables
- Updated runtime APIs with documentation and migration notes.
- Integration tests demonstrating runtime-driven submission through the Vulkan backend mock.
- Benchmarks comparing pre/post integration frame submission latency.

## Checklist
- [x] API surface documented in `docs/modules/runtime/README.md`.
- [x] New tests registered with CTest and passing on CI presets.
- [x] Benchmark results appended to this file.

## Benchmark Results

- Vulkan submission path average latency: `0.034 ms` per frame (50-iteration mean from `RuntimeHost.SubmitsRenderGraphThroughVulkanScheduler`).
