# Platform Module

## Current State

- Defines abstractions for filesystem access, input, windowing, and platform-specific utilities.
- Currently stubs out real OS integration pending backend work.

## Usage

- Link against `engine_platform` when consuming platform services from other modules; the target inherits `engine::project_options` and publishes headers via `engine::headers`.
- Implement platform-specific backends in the dedicated subdirectories as requirements mature.
- Validate backends with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to ensure GLFW options remain synchronised across toolchains.

## TODO / Next Steps

- Bind platform abstractions to OS windowing, input, and filesystem APIs.
