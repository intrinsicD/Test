# ADR-0003: Runtime Frame-Graph Contract

- **Status:** Proposed
- **Drivers:** `AI-003`, `RT-003`
- **Authors:** Rendering & Runtime Working Group
- **Last Updated:** 2025-02-17

## Context
Rendering backends require deterministic pass ordering, explicit resource transitions, and queue affinity metadata. The runtime must describe work without encoding backend-specific logic while still supporting validation and telemetry hooks.

## Decision
1. **Authoritative Frame-Graph Model:** `engine/rendering/frame_graph.hpp` defines nodes (passes) and resources. Passes declare read/write edges and queue preferences.
2. **Scheduler Interface:** Runtime submits graphs through an interface that backends implement. The interface exposes hooks for resource acquisition, command encoding, and telemetry collection.
3. **Metadata Expansion:** Pass descriptors include debug names, execution phases, and validation severity flags. These feed documentation, logging, and testing harnesses.
4. **Deterministic Serialisation:** Frame-graph instances serialize to a canonical form for caching, diffing, and reproducible builds.

## Consequences
- Runtime changes must update this ADR when new metadata categories are added.
- Backends can validate graphs before execution, enabling CI to catch misconfigured passes.
- Additional tooling (e.g., frame capture visualisers) can operate purely on the serialized representation.

## Follow-Up Work
- Document runtime integration steps in [`docs/tasks/T-0104-runtime-frame-graph-integration.md`](../tasks/T-0104-runtime-frame-graph-integration.md).
- Update `docs/modules/rendering/README.md` with scheduler implementation notes after the prototype stabilises.
