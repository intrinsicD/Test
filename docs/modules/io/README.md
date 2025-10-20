# IO Module

## Current State
- Provides geometry and animation import/export wrappers with format detection
  and a plugin-ready registry for mesh/point-cloud/graph handlers.
- Implements scaffolding for cache policies and integrates with asset caches.
- Supplies structured error handling as part of `DC-004` migration.

## Usage
- Build via `cmake --build --preset <preset> --target engine_io`.
- Include `<engine/io/api.hpp>` for import/export utilities and handler
  registration.
- Run `ctest --preset <preset> --tests-regex engine_io` and fuzz harnesses as
  they come online.

## TODO / Next Steps

- Track `IO-221`, `IO-230`, `IO-240` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — advances `RT-006` and `DC-004`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `IO-221` | Integrate signature database + fuzz harness (`RT-006`). | Signature set committed, fuzz target wired into CI, README updated. | 🔄 In Progress |
| `IO-230` | Publish structured error catalog. | Document error codes and remediation steps in README + design note. | 🟢 Todo |
| `IO-240` | Align telemetry for import/export failures. | Emit metrics consumed by diagnostics viewer and log failure provenance. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for timeline and dependencies.
