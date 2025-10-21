# Core Module

## Current State
- Wraps EnTT registry usage with a façade providing entity helpers and module
  discovery utilities consumed by higher-level systems.
- Hosts subsystem plugin interfaces and bootstrap plumbing used by `RuntimeHost`.
- Provides diagnostics utilities, configuration helpers, and foundational types
  shared across modules.
- Hosts the shared telemetry schema and references the
  [`Telemetry Instrumentation Guide`](../../design/telemetry_instrumentation_guide.md)
  used by downstream modules.

## Usage
- Build with `cmake --build --preset <preset> --target engine_core`.
- Include `<engine/core/module.hpp>` and related headers to access plugin
  registration and entity helpers.
- Run `ctest --preset <preset> --tests-regex engine_core`.

## TODO / Next Steps

- Track `CR-125`, `CR-130` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — maintains `DC-001` and supports `CC-001`.
- Next roadmap slice: `CR-135` dependency cycle diagnostics for subsystem plugins.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CR-118` | Draft diagnostics bridge specification (`CC-001`). | Publish design note for telemetry routing; link from module README. | ✅ Done |
| `CR-125` | Audit plugin lifecycle contracts (`DC-001`). | Document init/shutdown sequencing, add regression coverage. | ✅ Done |
| `CR-130` | Update configuration docs. | Refresh config API README section with latest presets and defaults. | ✅ Done |
| `CR-135` | Subsystem dependency diagnostics. | Detect cycles during registration and document remediation guidance. | 🟡 Planned |

Consult [ROADMAP.md](ROADMAP.md) for broader sequencing.

## Subsystem Lifecycle Contract

- `engine::core::plugin::ISubsystemInterface` implementers receive lifecycle callbacks in
  registration order. `initialize` is invoked sequentially; failures trigger shutdown of any
  previously started subsystems in reverse order to maintain `DC-001` determinism.
- `SubsystemLifecycleContext.runtime_name` mirrors the active runtime scene identifier.
  Treat the string view as non-owning; copy when persisting beyond the callback.
- `shutdown` is always invoked when initialization succeeds and when a later plugin fails.
  Implementations must be idempotent and `noexcept`. Regression coverage lives in
  `engine/runtime/tests/test_module.cpp` (see lifecycle tests).

## Configuration Defaults

- `RuntimeHostDependencies::streaming_config` defaults to two IO workers, queue capacity
  of 64, and async streaming enabled. Override per runtime by mutating the struct before
  constructing `RuntimeHost` or via subsystem configuration presets.
- Subsystem selection derives from either explicit `subsystem_plugins` or discovery through
  `SubsystemRegistry::load`. When `enabled_subsystems` is empty the runtime loads modules
  flagged `enabled_by_default` and resolves dependencies transitively.
- The runtime name (`RuntimeHostDependencies::scene_name`) feeds diagnostic output and the
  subsystem lifecycle context. Align naming across tooling and telemetry for consistent
  aggregation.
