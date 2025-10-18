# Runtime Module

## Current State
- Aggregates all engine subsystems through `RuntimeHost`, orchestrating animation playback, physics simulation, compute dispatching, and scene graph mirroring each tick.
- Exposes both C++ and C-compatible entry points for lifecycle control (`initialize`, `tick`, `shutdown`) and telemetry queries (joint names, body positions, dispatch timings).
- Maintains a default dependency bundle (`RuntimeHostDependencies`) that seeds sample controllers, meshes, and physics worlds for quick-start scenarios.
- The default mesh is a 128×128 subdivided quad (~16k vertices) so skinning telemetry exercises the 10k-vertex budget targeted by `RT-001`.
- Applies linear blend skinning during `geometry.deform` using cached rig bindings and animation-supplied joint transforms, falling back to uniform translation when skinning data is absent.
- When rendering is enabled, `RuntimeHostDependencies` also carries a default `rendering::components::RenderGeometry` descriptor and renderable debug name so the runtime can populate a scene entity for GPU submission.
- Discovers subsystem plugins through a `SubsystemRegistry`, loading enabled modules (and their dependencies) during runtime initialization.
- Accepts subsystem plugins through `RuntimeHostDependencies::subsystem_plugins`, invoking their lifecycle hooks during initialization, shutdown, and tick to support dependency-injected extensions.
- Exposes helper APIs (`configure_with_default_subsystems`, `default_subsystem_names`) and C bindings (`engine_runtime_configure_with_modules`) so hosts can choose which subsystems to load without manually constructing plugin instances.
- Exposes `RuntimeHost::RenderSubmissionContext` and `RuntimeHost::submit_render_graph` so embedders can feed the mirrored scene graph into the forward rendering pipeline and GPU scheduler (tested end-to-end against the Vulkan prototype).
- Configures the asynchronous IO thread pool via `RuntimeHostDependencies::streaming_config` and exposes `streaming_metrics()`
  alongside the C binding `engine_runtime_streaming_metrics` so diagnostics like
  `scripts/diagnostics/streaming_report.py` can track queue depth and request
  outcomes.
- Captures lifecycle telemetry through `RuntimeHost::diagnostics()`, mirroring the data via
  `engine_runtime_diagnostic_*` C bindings so scripts can analyse initialize/tick/shutdown
  timings and dispatcher stage statistics.
- Smoke tests in `engine/runtime/tests/` validate module registration; cross-module
  behaviour is exercised by the headless harness under
  [`engine/tests/integration`](../../../engine/tests/integration/README.md) which covers the
  animation/physics/runtime loop, asset-driven mesh loading, and runtime →
  rendering submissions (`TI-001`) now that Googletest fixture support from
  `T-0118` has landed.

## Usage
- Build via `cmake --build --preset <preset> --target engine_runtime`; this pulls every other module as a dependency.
- Include `<engine/runtime/api.hpp>` when hosting the runtime loop or embedding the engine; call `RuntimeHost::initialize()`/`tick()`/`shutdown()` to drive simulation.
- Adjust `RuntimeHostDependencies::streaming_config` before construction to tune
  asynchronous asset loading worker counts and queue depth.
- Inspect lifecycle timings via `RuntimeHost::diagnostics()` or the
  `engine_runtime_diagnostic_*` C functions; pair with
  `scripts/diagnostics/runtime_frame_telemetry.py` to export JSON suitable for
  dashboards.
- When bridging into rendering, construct a `RuntimeHost::RenderSubmissionContext` with your resource/material/scheduler providers and call `RuntimeHost::submit_render_graph(context)` after each `tick()`.
- Run `ctest --preset <preset> --tests-regex engine_runtime` to verify module plumbing before running broader integration suites.
- Toggle subsystem availability at configure time via the `ENGINE_ENABLE_<MODULE>` CMake options (for example `-DENGINE_ENABLE_RENDERING=OFF`). Disabled subsystems are omitted from the default registry but can still be re-enabled explicitly through the helper APIs.

## Diagnostics

- `RuntimeHost::diagnostics()` returns counters and timings for initialization, tick, and shutdown
  phases along with dispatcher stage statistics and subsystem lifecycle metrics.
- The C ABI mirrors these values via `engine_runtime_diagnostic_*` functions so external tooling can
  capture snapshots without linking against the C++ API.
- `scripts/diagnostics/runtime_frame_telemetry.py --frames 120 --dt 0.016` streams the dispatcher
  timings recorded in `compute::ExecutionReport` and now embeds the lifecycle metrics in its JSON
  output, making it suitable for dashboards that track regressions over time.

## Rendering Metadata Alignment Responsibilities

- Runtime submission code (`RuntimeHost::submit_render_graph`, the default forward-pipeline wiring, and test fixtures under
  `engine/runtime/tests/`) must stay byte-for-byte compatible with the rendering module's `FrameGraphResourceDescriptor`
  schema. Any change to resource formats, usage flags, queue affinity, or serialization fields in the rendering module requires
  a matching runtime update in the same change.
- Runtime reviewers share ownership for spotting rendering metadata changes. Treat the rendering README as the single source of
  truth for schema fields and cross-check every runtime submission path—including the integration suite under
  `engine/tests/integration/`—before approving metadata modifications.
- When runtime requires additional GPU resources (for example, expanding the forward pipeline with new attachments or compute
  passes), file a rendering task first, align on the metadata contract, and update both READMEs alongside the implementation so
  downstream teams understand who maintains which descriptors.
- Extend integration scenarios whenever metadata evolves so the Vulkan-backed vertical slice exercises the updated descriptors
  through `RuntimeHost::submit_render_graph`. This keeps `RT-003` parity intact across modules.

## Subsystem Plugin Contract

Subsystems implement `engine::core::plugin::ISubsystemInterface`. Each plugin exposes its
identity, declares dependencies, and reacts to lifecycle callbacks:

- `initialize(const SubsystemLifecycleContext&)` – invoked when the runtime boots.
- `shutdown(const SubsystemLifecycleContext&)` – invoked in reverse order during teardown.
- `tick(const SubsystemUpdateContext&)` – invoked once per frame with the timestep.

Dependencies are expressed by name via `dependencies()`. The `SubsystemRegistry` resolves the
transitive closure so dependent services are constructed in a deterministic order before they are
provided to the `RuntimeHost`.

`engine::runtime::make_default_subsystem_registry()` registers descriptors for every module enabled
via the `ENGINE_ENABLE_<MODULE>` CMake options (for example `ENGINE_ENABLE_RENDERING`). Hosts can
extend or replace the registry by registering additional descriptors prior to runtime
initialization.

Helper APIs defined in `engine/runtime/api.hpp` simplify configuration:

- `configure_with_default_subsystems()` resets the global `RuntimeHost` to the default registry.
- `configure_with_default_subsystems(std::span<const std::string_view>)` loads a subset of modules
  by name while automatically resolving dependencies.
- `default_subsystem_names()` enumerates the descriptors registered by default so hosts can present
  discovery UIs or validation diagnostics.
- The C bindings `engine_runtime_configure_with_default_modules()` and
  `engine_runtime_configure_with_modules()` mirror the C++ helpers for embedders that use the C ABI.

Projects with bespoke requirements can still assemble `RuntimeHostDependencies` manually, but the
helper APIs cover common cases such as disabling expensive subsystems or bootstrapping from a list
of module names.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
