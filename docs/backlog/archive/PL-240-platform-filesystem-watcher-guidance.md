# Backlog Item PL-240 — Platform Filesystem Watcher Guidance

- **Status**: Complete
- **Priority**: 3
- **Owner**: Platform Lead (with Assets/Tools collaboration)
- **Module(s)**: Platform, Assets, Tools
- **Goal**: Document platform-specific filesystem watcher behaviour and provide asset hot-reload integration examples so teams adopt the polling API consistently.

## Summary

The polling-based filesystem watcher underpins hot-reload flows across assets, tools, and runtime harness integrations. Contributors reported platform quirks (timestamp resolution, editor rename semantics, network share latency) that were missing from the documentation. PL-240 captures the operating-system caveats, prescribes polling cadence guidance, and illustrates how asset caches wire watcher callbacks so that downstream modules (sandbox UI, diagnostics shell) receive deterministic reload notifications.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Sequence platform/assets doc updates and ensure Roadmap alignment. | Agent Orchestrator (11) |
| Platform Lead | Curate watcher caveats, ensure guidance matches implementation. | Platform Lead |
| Assets Representative | Validate integration example against cache hot-reload flow. | Assets Lead |
| Tools Representative | Confirm sandbox UI expectations for reload events remain accurate. | Tools Lead |
| Docs/DevRel | Update module READMEs and cross-links; run documentation validation. | Docs/DevRel (95) |

## Definition of Done
- [x] Platform README documents filesystem watcher polling semantics, per-OS caveats, and path normalisation guarantees.
- [x] Assets README references the watcher API with concrete hot-reload wiring examples and troubleshooting guidance.
- [x] Roadmap/README next steps reflect the documentation update so future tasks build on the clarified guidance.
- [x] `scripts/validate_docs.py` passes after documentation changes.

## Dependencies
- [`AS-330`](AS-330-reference-dataset-packages.md) ingestion automation relying on watcher callbacks.
- [`TL-210`](TL-210-experiment-sandbox-ui.md) sandbox wiring that exposes reload telemetry.

## Related Artefacts
- [`engine/platform/include/engine/platform/filesystem/watcher.hpp`](../../../engine/platform/include/engine/platform/filesystem/watcher.hpp)
- [`engine/platform/src/filesystem/watcher.cpp`](../../../engine/platform/src/filesystem/watcher.cpp)
- [`engine/assets/src/mesh_asset.cpp`](../../../engine/assets/src/mesh_asset.cpp)

## Notes
- Polling cadence defaults to the caller; harness integrations typically poll once per frame. Higher frequencies tighten reload latency but increase filesystem traffic.
- Editors that swap files atomically (write-to-temp + rename) surface as `created` followed by `erased`. Documentation now calls this out alongside mitigation strategies for asset caches.
