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
3. **Metadata Expansion:** Resource descriptors capture `ResourceFormat`, `ResourceDimension`, `ResourceUsage`, and explicit initial/final `ResourceState` transitions. Pass descriptors include debug names, execution phases (`PassPhase`), validation severity flags (`ValidationSeverity`), and queue affinity hints. These feed documentation, logging, scheduling, and testing harnesses.
4. **Deterministic Serialisation:** Frame-graph instances serialize to a canonical JSON document for caching, diffing, and reproducible builds. The serializer emits resources, passes, and the execution order using stable declaration orderings and escaped debug names so identical graphs always yield byte-for-byte identical payloads.

## Consequences
- Runtime changes must update this ADR when new metadata categories are added.
- Backends can validate graphs before execution, enabling CI to catch misconfigured passes.
- Additional tooling (e.g., frame capture visualisers) can operate purely on the serialized representation.

## Follow-Up Work
- Document runtime integration steps in [`docs/tasks/T-0104-runtime-frame-graph-integration.md`](../tasks/T-0104-runtime-frame-graph-integration.md).
- Update `docs/modules/rendering/README.md` with scheduler implementation notes after the prototype stabilises.
