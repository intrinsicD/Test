# Platform Module Roadmap

_Last Updated: 2025-03-04_

## Roadmap

| Task ID | Description | Status |
| --- | --- | --- |
| `PL-215` | Document SDL backend parity requirements and validation steps. | ✅ Complete |
| `PL-222` | Deliver filesystem watcher abstraction for hot reload (`CC-002`). | ✅ Complete |
| `PL-230` | Refresh backend selection guidance in docs and presets. | ✅ Complete |

## Schedule

| Milestone | Tasks | Target |
| --- | --- | --- |
| Sprint 1 | `PL-215` | ✅ Delivered 2025-03-31 |
| Sprint 2 | `PL-230` | ✅ Delivered 2025-03-04 |

Coordinate watcher work with Tools (`TL-101`) to ensure hot reload flows remain
integrated end-to-end now that Assets (`AS-315`) consumes the watcher
abstraction and exports telemetry for diagnostics consumers.

## Notes

- 2025-10-28: GLFW backend now satisfies headless automation via hidden-window support; SDL parity tasks tracked under `DC-003`
  are deferred until the backlog reprioritises them.
- 2025-10-23: CMake now disables `ENGINE_ENABLE_GLFW` when GLFW cannot be
  configured so presets fall back to the mock backend without breaking builds.
