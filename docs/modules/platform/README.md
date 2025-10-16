# Platform Module

## Current State
- Abstracts window management via backend-agnostic interfaces (`Window`, `EventQueue`, `SwapchainSurface`) with implementations for GLFW, SDL, and a headless mock.
- Provides window console logging, filesystem helpers, and input state tracking consumed by editor/runtime tooling.
- Exposes backend selection enums and configuration (`WindowBackend`) together with capability requirements on `WindowConfig` so callers can demand headless-safe or native-surface backends.
- Tests in `engine/platform/tests/` exercise filesystem wrappers, window console behaviour, input state transitions, and backend selection across mock/SDL/auto configurations.

## Usage
- Build with `cmake --build --preset <preset> --target engine_platform`; ensure the GLFW third-party dependency is fetched/configured.
- Include `<engine/platform/windowing/window.hpp>` and related headers to create windows or query input state; link against `engine_platform`.
- Run `ctest --preset <preset> --tests-regex engine_platform` (with display backends mocked in CI) to validate behaviour.

## Configuration
- Select the default runtime backend at configure time via `-DENGINE_WINDOW_BACKEND=<GLFW|SDL|MOCK>`. Presets default to `GLFW` while CI and headless scenarios can override to `MOCK`.
- Override the selection at runtime with the `ENGINE_PLATFORM_WINDOW_BACKEND` environment variable (`auto`, `mock`, `glfw`, `sdl`). Overrides are validated against the capability requirements declared on the `WindowConfig`.
- Use `WindowConfig::capability_requirements` to demand `require_headless_safe` or `require_native_surface`. Automatic selection filters out backends that cannot satisfy these flags and raises descriptive errors when an explicit backend is incompatible.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
