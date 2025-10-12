# Core Module

## Current State
- Wraps the EnTT registry with the engine-facing `engine::core::ecs::registry` façade, exposing typed entity/component management plus debug UI helpers for inspection.
- Provides module discovery helpers (`module_name`) and scaffolding for runtime subsystems (configuration, diagnostics, plugin, and memory namespaces are staged for expansion).
- Tests under `engine/core/tests/` validate the ECS façade and shared entry points.

## Usage
- Build the module via `cmake --build --preset <preset> --target engine_core`; this links against EnTT, spdlog, and Dear ImGui from third_party.
- Include `<engine/core/ecs/registry.hpp>` to manage entities and `<engine/core/api.hpp>` for module exports.
- Execute `ctest --preset <preset> --tests-regex engine_core` with testing enabled to ensure registry semantics remain intact.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
