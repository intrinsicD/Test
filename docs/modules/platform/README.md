# Platform Module

## Current State
- Provides virtual filesystem providers, deterministic backend selection
  plumbing, and mocked window/input services pending concrete OS integrations.
- Exposes a cross-platform filesystem watcher abstraction powering cache hot
  reload flows across modules.
- Assets module consumes the watcher to resubscribe mesh/graph/point cloud/
  shader/texture caches on load and unregister on unload.
- Supports configuration via `ENGINE_WINDOW_BACKEND` and the runtime
  `ENGINE_PLATFORM_WINDOW_BACKEND` override with deterministic fallbacks.

## Usage
- Build with `cmake --build --preset <preset> --target engine_platform`.
- Include `<engine/platform/windowing/window.hpp>` to construct
  `WindowConfig` instances and inspect backend capability requirements.
- Include `<engine/platform/windowing/window_system.hpp>` to access backend
  selection and window creation APIs.
- Include `<engine/platform/filesystem/watcher.hpp>` to subscribe to file
  change notifications; drive `FilesystemWatcher::poll()` from the engine's
  main loop or an IO thread to surface hot reload events.
- Run `ctest --preset <preset> --tests-regex engine_platform`.

## Backend Selection Reference (`PL-230`)

Backend selection couples a build-time default with a runtime override while
preserving deterministic fallbacks. The implementation lives in
`engine/platform/windowing/window.hpp` and
`engine/platform/src/windowing/window_system.cpp`.

### Build-Time Defaults

- Configure the preferred backend at configure time via
  `-DENGINE_WINDOW_BACKEND=<GLFW|SDL|MOCK>`.
- The cache entry is normalised to upper-case and stored as
  `ENGINE_PLATFORM_DEFAULT_BACKEND` inside the generated build. Setting the
  value to `AUTO` disables the build-time preference.
- Presets under `scripts/build/presets/*.json` keep the default aligned with the
  target platform (e.g., Linux desktop = GLFW, CI/headless = MOCK).

### Runtime Override

- `ENGINE_PLATFORM_WINDOW_BACKEND` controls runtime selection. Accepted values:
  `auto`, `glfw`, `sdl`, and `mock` (case-insensitive, surrounding whitespace
  ignored).
- Invalid values are treated as a request for the mock backend so headless
  automation still launches successfully.
- When a non-mock override is supplied the selector still appends mock as a
  safety fallback to keep tests deterministic if capability requirements are not
  satisfied.

### Capability Filtering

`WindowConfig::CapabilityRequirements` constrains the candidate list:

| Capability | Description | Eligible Backends |
| --- | --- | --- |
| `require_headless_safe` | Allow running without a display server. | SDL, Mock |
| `require_native_surface` | Provide a native swapchain surface handle. | GLFW, SDL |

Backends violating the required capabilities are removed before selection and
produce explicit error messages when targeted directly.

### Automatic Fallback Order

When `WindowBackend::Auto` is requested the selector evaluates candidates in the
following order, skipping anything unavailable or capability-incompatible:

1. Runtime override (`ENGINE_PLATFORM_WINDOW_BACKEND`), then mock if the
   override was not already mock.
2. Build-time default (`ENGINE_WINDOW_BACKEND`) when it resolves to an available
   backend.
3. Remaining compiled backends in a deterministic order (GLFW → SDL → Mock,
   guarded by `ENGINE_PLATFORM_HAS_*` macros).

Each attempt records failure messages; an exception summarising all failed
backends is raised when no candidate succeeds. Integrations should surface the
message to ease troubleshooting.

## SDL Backend Parity Checklist (`PL-215`)

- Follow the [SDL backend parity checklist](sdl_backend_checklist.md) to stage
  work on a native SDL window implementation. The document summarises
  prerequisites, validation steps, and remaining gaps relative to the GLFW
  backend so `DC-003` adopters can plan implementation tasks without losing
  determinism or observability guarantees.

## TODO / Next Steps

- Track `PL-215` follow-ups using the SDL parity checklist (backend feature
  parity, CI coverage, troubleshooting docs) and keep status synced with the
  [central roadmap](../../ROADMAP.md) while `DC-003` implementation tasks are
  staffed.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `PL-215` | Publish SDL backend parity checklist (`DC-003`). | Document feature parity, dependencies, and validation steps. | ✅ Done |
| `PL-222` | Implement filesystem watcher abstraction (`CC-002`). | Provide cross-platform watcher with tests and README updates. | ✅ Done |
| `PL-230` | Update backend selection docs. | Refresh README + root docs with backend selection flow. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for more detail.
