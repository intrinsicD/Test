# Platform Module

## Current State

- Defines abstractions for filesystem access, input, windowing, and platform-specific utilities.
- Provides sandboxed filesystem providers with a virtual mounting layer while
  windowing/input continue to use mock backends until OS integrations land.

## Usage

- Link against `engine_platform` when consuming platform services from other modules; the target inherits `engine::project_options` and publishes headers via `engine::headers`.
- Implement platform-specific backends in the dedicated subdirectories as requirements mature.
- Validate backends with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to ensure GLFW options remain synchronised across toolchains.
- Configure the automatic backend selection by setting `ENGINE_PLATFORM_WINDOW_BACKEND` (`auto`, `mock`, `glfw`, or `sdl`).

## TODO / Next Steps

- Bind platform abstractions to real OS windowing and input APIs and expand the
  filesystem layer with write/watch capabilities.
