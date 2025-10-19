# Scene Module

## Current State
- Wraps EnTT to provide high-level `Scene` and `Entity` abstractions with hierarchy management, component helpers, and registry access.
- Defines canonical component sets (name, transforms, hierarchy, etc.) plus system implementations for hierarchy propagation and transform updates.
- Supports scene serialization/deserialization and integrates with runtime for entity lifecycle management.
- Tests in `engine/scene/tests/` cover component semantics, system updates, serialization, and destruction order.

## Usage
- Build via `cmake --build --preset <preset> --target engine_scene`; this links against `engine_core` and `engine_math`.
- Include `<engine/scene/scene.hpp>` and component headers to create/manipulate scenes; call systems in `<engine/scene/systems.hpp>` to update transforms.
- Run `ctest --preset <preset> --tests-regex engine_scene` to validate component/system behaviour after changes.

## Validation
- Invoke `scene::validation::validate_hierarchy(scene)` or the registry overload to
  generate a `HierarchyValidationReport` that captures cycle detection, dangling
  parents, non-finite transforms, and world/local mismatches.
- Configure tolerances with `HierarchyValidationOptions` when working with
  alternative numerical thresholds or when you need to include dirty entities in
  the analysis.
- Runtime integration records the latest report in
  `RuntimeDiagnostics::scene_validation` and exposes aggregate counts through
  the C API helpers (`engine_runtime_diagnostic_scene_*`). Use the telemetry
  pipeline or the new C ABI entry points to surface hierarchy health in tooling.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
