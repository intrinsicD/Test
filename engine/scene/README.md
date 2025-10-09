# Scene Module

## Current State

- Outlines the scene graph, component storage, serialization, and system execution layout.
- Currently focuses on structure and scaffolding pending concrete implementations.

## Usage

- Link against `engine_scene` to host world state once components and systems are implemented; the target inherits `engine::project_options` and publishes headers through `engine::headers` for dependent modules.
- Keep serialization and graph updates coordinated with runtime requirements.
- Build and test via the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to track integration regressions.

## TODO / Next Steps

- Implement scene graph serialization, systems, and runtime traversal.
