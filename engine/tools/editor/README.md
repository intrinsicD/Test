# Engine Tools Editor

## Current State

- Contains scaffolding files that will evolve alongside the subsystem.
- Depends on runtime scene graph, rendering viewport, and input abstractions.

## Objectives

- Provide a dockable, real-time editor for scene authoring and debugging.
- Maintain parity between editor representations and runtime data structures to avoid divergent code paths.

## Roadmap Alignment

- **Phase 0** – Reuse tooling common library for windowing, menus, and theming.
- **Phase 3** – Deliver the first interactive shell: hierarchy, inspector, asset browser, and viewport.
- **Phase 4** – Package as a standalone application with plugin discovery and project templates.

## Immediate Next Steps

1. Audit runtime services for editor-facing APIs (entity queries, component registration, hot-reload hooks).
2. Define UI layout wireframes (dockspace, panels) and align with Dear ImGui guidelines.
3. Prototype viewport rendering path that embeds runtime frame output within the editor window.
4. Specify undo/redo architecture leveraging command stacks and scene serialization snapshots.
5. Draft integration tests that load sample scenes and validate editor actions against runtime state.

## Dependencies & Open Questions

- Requires scene component metadata (naming, reflection) to drive property inspectors.
- Needs deterministic serialization format for persistence and collaboration workflows.
- Evaluate scripting integration (Lua/Python) for extending editor tooling.
