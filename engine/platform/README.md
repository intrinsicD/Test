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

The module already exposes a useful façade (filesystem sandboxes, window
selection, mocked input) but still relies on stubbed backends. The immediate
roadmap focuses on turning those abstractions into production-grade services
before other subsystems (rendering, runtime, tooling) depend on them.

1. **Filesystem service maturity**
   - Add write helpers (`write_text`, `write_binary`) and streaming interfaces
     so authoring tools can persist data through the sandbox.
   - Expose directory enumeration and change notifications (using
     `std::filesystem::directory_iterator` plus OS-specific watchers) to unlock
     hot-reload workflows.
   - Harden error reporting by returning structured status codes instead of
     silently falling back to `std::nullopt`.
   - Extend `VirtualFilesystem` so mounts can be flagged read/write and so that
     it can forward writes/watch events to the correct provider.

2. **Windowing backend integration**
   - Replace the mock factory with concrete GLFW/SDL implementations that create
     native windows, pump the OS message loop, and manage swapchain-compatible
     surfaces.
   - Honour `ENGINE_PLATFORM_WINDOW_BACKEND` overrides while detecting backend
     availability at runtime (GLFW initialisation, SDL subsystem checks).
   - Provide feature flags for fullscreen, high-DPI, and context creation
     options that rendering will require.
   - Share a common adapter layer that translates backend-specific events into
     the module’s `Event`/`EventQueue` types.

3. **Input pipeline wiring**
   - Hook real keyboard/mouse events from the windowing backends into
     `input::InputState`, including delta cursor motion and scroll accumulation.
   - Introduce device descriptors (gamepads, touch, pens) so higher layers can
     query capabilities.
   - Add per-frame sampling utilities that feed both synchronous polling and
     event-driven consumers.

4. **Testing and diagnostics**
   - Author integration tests that spawn headless/backbuffer windows and verify
     backend fallbacks work across platforms.
   - Build soak tests for the filesystem watcher to ensure stability under rapid
     churn.
   - Surface debug logging toggles to ease backend triage (initialisation
     failures, device enumeration results).

Keep the aggregated backlog in the workspace root aligned with this plan when
milestones are delivered.
