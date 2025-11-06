---
id: RG-450-A
title: Node descriptor API design
status: review
priority: P1
area: rendering
size: S
owner: rendering-lead
gates: [docs]
relates_to: [bundle:A, RG-450]
blocked_on: []
links: ["docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md", "docs/design/RG_450_NODE_DESCRIPTOR_API.md", "hybrid_workflow/backlog/RG-450-modular-render-pipeline.md"]
---
# Task RG-450-A — Node Descriptor API Design
## Intent
Design the node descriptor API so plugin-provided render passes can self-describe their resource contracts and queue preferences.
## Steps
1. [x] Create subtask from RG-450
2. [x] Document node descriptor structures in design doc
   - Created `docs/design/RG_450_NODE_DESCRIPTOR_API.md` with comprehensive specification
3. [x] Define INode interface with reflection contracts
   - Documented Reflect/Compile/Execute phase semantics
   - Specified NodeContext API for compilation and execution
4. [x] Create example node implementations
   - 4 example nodes: Geometry, Bloom (compute), Deferred Lighting, Swapchain Output
   - Each demonstrates different patterns (single/multi-resource, async compute, external)
5. [ ] Review API with module leads
6. [ ] Update parent RG-450 with progress

## Progress Notes

**2025-11-06:** API design document completed (`docs/design/RG_450_NODE_DESCRIPTOR_API.md`)
- Documented node descriptor validation rules (compile-time and runtime)
- Specified INode interface semantics with phase contracts
- Created 4 example node implementations covering common patterns
- Designed tag-based pipeline profile system
- Documented hot-reload support and plugin integration
- Outlined validation rules and error diagnostics
- Created 5-phase implementation checklist
- Ready for review with module leads

