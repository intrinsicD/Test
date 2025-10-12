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

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
