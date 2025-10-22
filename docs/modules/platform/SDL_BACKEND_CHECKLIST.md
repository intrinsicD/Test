# SDL Backend Parity Checklist

`PL-215` documents the work required to bring the SDL window backend to feature
parity with the GLFW path that currently ships as the default desktop
configuration. It aligns with the architecture initiative `DC-003` by capturing
installation requirements, validation steps, and the gaps between the existing
headless stub and a full SDL integration.

## Current Status Snapshot

- The compiled backend today is a stub built on `HeadlessWindow`, implemented in
  [`engine/platform/src/windowing/sdl_window.cpp`](../../../engine/platform/src/windowing/sdl_window.cpp).
  It mirrors window lifecycle semantics but does **not** create a native SDL
  window or dispatch events through SDL APIs.
- SDL support remains optional at configure time; presets default to GLFW on
  desktop targets and fall back to the mock backend for CI/headless runs.
- No SDL-specific regression tests or CI jobs execute today. The checklist below
  enumerates the coverage expected once parity lands.

## Parity Targets

Delivering parity requires the SDL backend to satisfy the following contracts:

1. **Window Lifecycle** – Create and destroy native SDL windows, respecting the
   configuration declared via `WindowConfig` (dimensions, title, fullscreen,
   DPI awareness, vsync hints).
2. **Event Pumping** – Translate SDL events (window, input, controller, text)
   into the engine `EventQueue` while preserving deterministic ordering for
   replay-driven tests.
3. **Surface Integration** – Expose `SwapchainSurface` handles compatible with
   Vulkan (and future DirectX/Metal backends) so rendering schedulers can share
   code paths with GLFW.
4. **Headless Safety** – Support the mock/headless mode used in CI by compiling
   the stub implementation and allowing runtime selection to demote to the mock
   backend when SDL prerequisites are missing.
5. **Telemetry Hooks** – Emit structured logs and metrics for initialization
   failures, capability mismatches, and runtime errors, following the patterns
   documented for GLFW in the platform module README.
6. **Configuration Parity** – Honour the same environment variables and preset
   overrides (`ENGINE_WINDOW_BACKEND`, `ENGINE_PLATFORM_WINDOW_BACKEND`) as the
   existing backends, including deterministic fallback order when SDL is
   unavailable.

## Prerequisites

| Platform | Packages / Notes |
| --- | --- |
| **Linux** | Install SDL development headers (SDL2 ≥ 2.30 or SDL3) via the
  distribution package manager (e.g., `libsdl2-dev` on Debian/Ubuntu or
  `sdl2` on Fedora/Arch). Ensure Wayland/X11 dependencies match the selected
  rendering backend. |
| **Windows** | Install the official SDL developer package and add the
  `SDL3.dll`/`SDL2.dll` directory to the PATH for the selected generator. |
| **macOS** | Install SDL via Homebrew (`brew install sdl2` or `sdl3`), and
  grant windowing permissions when prompted. |

General requirements:

- CMake ≥ 3.20 and a C++20-capable compiler (Clang ≥ 22, GCC ≥ 12, MSVC ≥ 19.34).
- The platform presets must be updated to locate SDL headers and libraries via
  `CMAKE_PREFIX_PATH` or dedicated cache entries (`SDL2_DIR`, `SDL3_DIR`).
- Validation runs should enable the SDL runtime loader (`SDL_DYNAMIC_API`
  environment variable) when using the modular SDL3 builds.

## Build Configuration

1. Configure with SDL enabled and select the backend during configuration:

   ```bash
   cmake --preset linux-gcc-debug \
     -DENGINE_ENABLE_PLATFORM=ON \
     -DENGINE_WINDOW_BACKEND=SDL \
     -DENGINE_PLATFORM_SDL_ROOT=$HOME/sdk/SDL
   ```

   - Provide an explicit `ENGINE_PLATFORM_SDL_ROOT` (or platform-specific cache
     entries) when SDL is not installed in a default system location.
   - Retain `ENGINE_WINDOW_BACKEND=MOCK` for CI presets that cannot access a
     display server; runtime overrides will still allow targeted SDL coverage.

2. Build the platform module and associated tests:

   ```bash
   cmake --build --preset linux-gcc-debug --target engine_platform
   cmake --build --preset linux-gcc-debug --target engine_platform_tests
   ```

3. For runtime validation, ensure the integration tests honour the SDL backend:

   ```bash
   ENGINE_PLATFORM_WINDOW_BACKEND=sdl \
     ctest --preset linux-gcc-debug --tests-regex engine_integration_tests
   ```

## Validation Checklist

- ✅ **Unit Tests** – `ctest --preset <preset> --tests-regex engine_platform`
  passes with SDL selected at build time. Extend coverage with fixture tests
  that validate event translation and swapchain surface creation.
- ✅ **Integration Tests** – Runtime integration suites run with
  `ENGINE_PLATFORM_WINDOW_BACKEND=sdl`, exercising the full
  animation/physics/runtime/rendering loop without regressions.
- ✅ **Manual QA** – Launch a sample runtime (or minimal repro harness) to
  confirm window creation, resize/fullscreen toggles, controller input, and
  DPI-aware rendering on each target OS.
- ✅ **Telemetry** – Diagnostics viewer displays SDL initialization failures,
  capability mismatches, and runtime errors with actionable guidance.
- ✅ **Fallback Behaviour** – Removing or misconfiguring SDL libraries should
  deterministically demote the runtime to the mock backend and surface
  structured error messages.

## Telemetry & Troubleshooting

- Surface SDL loader failures through the existing platform logging facility and
  map them onto `engine::platform::WindowErrorCode` values for consumption by
  runtime diagnostics.
- Record per-frame event pump timings alongside backend identifiers so CI can
  detect regressions relative to the GLFW baseline.
- Capture SDL version information and enabled subsystems during startup to aid
  support requests.

## Parity Gaps & Follow-Up

- Implement native window creation (`SDL_CreateWindow`, `SDL_CreateWindowFrom`)
  and hook swapchain surface export for Vulkan, DirectX 12, and Metal.
- Port the event translation layer to cover mouse, keyboard, controller, touch,
  and window events while maintaining determinism guarantees.
- Update CI presets to install SDL packages on Linux/macOS runners and provide
  PATH setup on Windows builders.
- Author integration smoke tests that toggle backends via runtime overrides to
  prevent regressions in fallback logic.
- Extend the platform README with SDL-specific troubleshooting once the backend
  ships, and add cookbook entries for interacting with SDL window handles from
  higher-level systems (editor tooling, ImGui overlays).
