# Runtime Module

## Current State

- Provides the aggregation layer that stitches animation, physics, compute, and geometry into an executable runtime loop.
- Mirrors the active joint transforms into an `engine::scene::Scene` so callers can inspect a synchronized scene graph.
- Exposes a `RuntimeHost` that owns subsystem dependencies, enforces explicit `initialize()`/`shutdown()` semantics, and
  caches the most recent `runtime_frame_state` (including per-kernel dispatch timings) for inspection.

## Usage

- Link against `engine_runtime` to bootstrap the engine once subsystems are wired together; the target inherits `engine::project_options` and relies on the header aggregator `engine::headers` to surface subsystem APIs.
- Construct a `RuntimeHost` when you need explicit lifecycle control or dependency injection:

  ```cpp
  engine::runtime::RuntimeHost host{engine::runtime::RuntimeHostDependencies{}};
  host.initialize();
  const auto frame = host.tick(1.0 / 60.0);
  host.shutdown();
  ```
- Expand runtime orchestration logic under `src/` and keep tests aligned with subsystem growth.
- Validate integrations with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) and extend coverage as new subsystems land.

## TODO / Next Steps

- **Near term (1–2 milestones)** – With `RuntimeHost` in place, extend the dispatcher beyond the fixed kernel chain, add lifecycle telemetry, and harden scene mirroring for dynamic rigs.
- **Mid term (3–5 milestones)** – Introduce asynchronous asset streaming, render scheduling hooks, and telemetry surfaces that can feed profiling tools.
- **Long term (5+ milestones)** – Integrate with the engine-wide job system, support deterministic replay/state capture, and add hot-reloadable configuration. See [docs/design/runtime_plan.md](../../docs/design/runtime_plan.md) and the [global alignment overview](../../docs/global_roadmap.md) for the full roadmap and status tracking.
