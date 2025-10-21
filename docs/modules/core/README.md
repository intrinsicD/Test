# Core Module

## Current State
- Wraps EnTT registry usage with a façade providing entity helpers and module
  discovery utilities consumed by higher-level systems.
- Hosts subsystem plugin interfaces and bootstrap plumbing used by `RuntimeHost`.
- Provides diagnostics utilities, configuration helpers, and foundational types
  shared across modules.
- Hosts the shared telemetry schema and references the
  [`Telemetry Instrumentation Guide`](../../design/telemetry_instrumentation_guide.md)
  used by downstream modules.

## Usage
- Build with `cmake --build --preset <preset> --target engine_core`.
- Include `<engine/core/module.hpp>` and related headers to access plugin
  registration and entity helpers.
- Run `ctest --preset <preset> --tests-regex engine_core`.

## TODO / Next Steps

- Track `CR-125`, `CR-130` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — maintains `DC-001` and supports `CC-001`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CR-118` | Draft diagnostics bridge specification (`CC-001`). | Publish design note for telemetry routing; link from module README. | ✅ Done |
| `CR-125` | Audit plugin lifecycle contracts (`DC-001`). | Document init/shutdown sequencing, add regression coverage. | 🟢 Todo |
| `CR-130` | Update configuration docs. | Refresh config API README section with latest presets and defaults. | 🟢 Todo |

Consult [ROADMAP.md](ROADMAP.md) for broader sequencing.
