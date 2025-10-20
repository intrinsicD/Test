# Platform Module

## Current State
- Provides virtual filesystem providers, backend selection plumbing, and mocked
  window/input services pending concrete OS integrations.
- Supports configuration via `ENGINE_WINDOW_BACKEND` and related presets.

## Usage
- Build with `cmake --build --preset <preset> --target engine_platform`.
- Include `<engine/platform/windowing/window_system.hpp>` to access backend
  selection and window creation APIs.
- Run `ctest --preset <preset> --tests-regex engine_platform`.

## TODO / Next Steps

- Track `PL-215`, `PL-222`, `PL-230` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — tied to `DC-003` and `CC-002`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `PL-215` | Publish SDL backend parity checklist (`DC-003`). | Document feature parity, dependencies, and validation steps. | 🔄 In Progress |
| `PL-222` | Implement filesystem watcher abstraction (`CC-002`). | Provide cross-platform watcher with tests and README updates. | 🟢 Todo |
| `PL-230` | Update backend selection docs. | Refresh README + root docs with backend selection flow. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for more detail.
