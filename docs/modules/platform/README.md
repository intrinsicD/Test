# Platform Module

## Current State
- Abstracts window management via backend-agnostic interfaces (`Window`, `EventQueue`, `SwapchainSurface`) with implementations for GLFW, SDL, and a headless mock.
- Provides window console logging, filesystem helpers, and input state tracking consumed by editor/runtime tooling.
- Offers backend selection enums and configuration (`WindowConfig`, `WindowBackend`) to control creation semantics.
- Tests in `engine/platform/tests/` exercise filesystem wrappers, window console behaviour, and input state transitions.

## Usage
- Build with `cmake --build --preset <preset> --target engine_platform`; ensure the GLFW third-party dependency is fetched/configured.
- Include `<engine/platform/windowing/window.hpp>` and related headers to create windows or query input state; link against `engine_platform`.
- Run `ctest --preset <preset> --tests-regex engine_platform` (with display backends mocked in CI) to validate behaviour.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
