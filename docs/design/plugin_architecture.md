# Runtime Plugin Architecture

Subsystem plugins exposed through `engine::core::plugin::ISubsystemInterface` enable
modules to opt into runtime orchestration without direct linking. This document captures
the authoritative lifecycle expectations aligned with `DC-001`.

## Lifecycle Sequencing

- Plugins are initialized sequentially in the order they are registered with the runtime
  host or resolved from `SubsystemRegistry`. Each call receives a
  `SubsystemLifecycleContext` whose `runtime_name` mirrors the active scene/runtime ID.
- If any plugin throws during `initialize`, the runtime host immediately shuts down all
  previously initialized plugins in reverse order before rethrowing. This preserves
  determinism and prevents partially initialized subsystems from leaking resources.
- `shutdown` is invoked exactly once per successful initialization and is guaranteed to
  execute during normal teardown and after initialization failures. Implementations must be
  `noexcept` and idempotent.
- `tick` follows the same registration order on every frame. Subsystems should treat the
  supplied `SubsystemUpdateContext::delta_time` as authoritative.

## Configuration Integration

- `RuntimeHostDependencies::subsystem_plugins` accepts explicit plugin instances for tests
  and bespoke compositions. When empty, the runtime consults `SubsystemRegistry`, honoring
  `enabled_subsystems` or the default-enabled descriptors.
- `RuntimeHostDependencies::scene_name` feeds diagnostics, telemetry labels, and the
  lifecycle context. Copy the value if it must outlive the callback.
- IO thread pool configuration (`core::threading::IoThreadPoolConfig`) is applied before
  plugin initialization begins. Failures during initialization unwind the configuration via
  `IoThreadPool::shutdown()`.

For examples and test coverage consult
`engine/runtime/tests/test_module.cpp` (lifecycle regression tests) and the
Core module README which documents configuration defaults.
