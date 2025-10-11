# Runtime Module

## Current State

- Provides the aggregation layer that stitches animation, physics, compute, and geometry into an executable runtime loop.
- Mirrors the active joint transforms into an `engine::scene::Scene` so callers can inspect a synchronized scene graph.
- Includes scaffolding for lifecycle management and module loading.

## Usage

- Link against `engine_runtime` to bootstrap the engine once subsystems are wired together; the target inherits `engine::project_options` and relies on the header aggregator `engine::headers` to surface subsystem APIs.
- Expand runtime orchestration logic under `src/` and keep tests aligned with subsystem growth.
- Validate integrations with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) and extend coverage as new subsystems land.

## TODO / Next Steps

- **Near term (1–2 milestones)** – Formalise lifecycle ownership, expose deterministic lifecycle APIs, extend the dispatcher beyond the fixed kernel chain, and harden scene mirroring for dynamic rigs.
- **Mid term (3–5 milestones)** – Introduce asynchronous asset streaming, render scheduling hooks, and telemetry surfaces that can feed profiling tools.
- **Long term (5+ milestones)** – Integrate with the engine-wide job system, support deterministic replay/state capture, and add hot-reloadable configuration. See [docs/design/runtime_plan.md](../../docs/design/runtime_plan.md) for the full roadmap and status tracking.
