# Runtime Module Roadmap

_Last Updated: 2025-10-20_

## Goals

| Goal | Description | Status |
| --- | --- | --- |
| Submission parity (`RU-307`) | Align runtime submission with Vulkan backend (`RT-003`). | ✅ Done |
| Streaming telemetry (`RU-315`) | Surface async queue metrics in runtime diagnostics. | ✅ Done |
| Diagnostics docs (`RU-320`) | Refresh troubleshooting guide with latest instrumentation. | ✅ Done |

## Active Task

Runtime-specific milestones from `RT-005` are complete. Track cross-cutting
efforts under `AI-002` and `CC-001` for the next iteration of diagnostics
instrumentation.

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `AI-002` | Expand async streaming diagnostics with per-asset attribution. | Assets module hot reload callbacks (`CC-002.2`). |

Ensure updates are mirrored in task records (`T-0104`) and the central roadmap.
