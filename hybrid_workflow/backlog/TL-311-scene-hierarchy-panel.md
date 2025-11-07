---
id: TL-311
title: Scene hierarchy diagnostic panel
status: new
priority: P2
area: tools
size: M
owner: tools-lead
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on: ["TL-310 editor foundations (in progress)", "TL-310-2a Application rendering (needed for panel visualization)"]
links:
  - "hybrid_workflow/backlog/TL-310-editor-foundations.md"
  - "hybrid_workflow/backlog/TL-310-2a-application-presentation-integration.md"
  - "hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md"
  - "docs/modules/tools/README.md"
  - "docs/modules/scene/README.md"
  - "docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md"
---

# Task TL-311 — Scene Hierarchy Diagnostic Panel

## Intent

Deliver an interactive scene hierarchy panel inside the revived editor so tools and runtime teams can inspect and manipulate the entity graph without leaving the hybrid workflow harness.

---

## Context

**Current State:**
- [`TL-310`](TL-310-editor-foundations.md) is rebuilding the editor harness and panel registry, but no concrete panels ship with the baseline.
- The tools module documentation highlights Dear ImGui helpers and panel registry support, yet there is no task covering an entity hierarchy view.
- [`TOOLS_USAGE_ANALYSIS.md`](TOOLS_USAGE_ANALYSIS.md) flags the lack of panel implementation follow-ups as an adoption gap.

**Desired State:**
- The editor exposes a deterministic tree view of the runtime `scene::Scene`, reflecting parent/child relationships and selection state.
- Tooling can highlight runtime selections, inspect component metadata, and trigger validation workflows in sync with the harness.
- Documentation and weekly demo cadence capture the new panel as part of Bundle B deliverables.

**References:**
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — module invariants and ImGui integration guidance.
- [`docs/modules/scene/README.md`](../../docs/modules/scene/README.md) — hierarchy model, validation rules, and transform propagation guarantees.
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) — runtime ↔ tooling synchronisation contract.
- [`hybrid_workflow/backlog/TL-310-editor-foundations.md`](TL-310-editor-foundations.md) — parent task restoring the panel registry.
- [`hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md`](TOOLS_USAGE_ANALYSIS.md) — workflow analysis recommending TL-311–TL-314 follow-ups.

---

## Design / Plan

### Constraints

- Honour the lifecycle hooks defined in `PanelRegistry` so registration/unregistration stays deterministic.
- Mirror the runtime scene graph without mutating simulation state unless the user explicitly issues commands.
- Keep hierarchy traversal incremental to avoid exceeding the 1 ms/frame tooling budget documented in the tools module README.
- Surface validation results from `scene::validation::validate_hierarchy` without blocking the UI thread.
- Update roadmap/backlog artefacts when the panel lands to keep Bundle B tracking accurate.

### API / Data Sketch

```cpp
struct HierarchyPanelModel {
  engine::scene::Scene* scene = nullptr;
  entt::entity selected = entt::null;
  void synchronize_selection(entt::entity entity_id);
  void for_each_child(entt::entity parent, const ChildVisitor& visitor) const;
};

class SceneHierarchyPanel : public engine::tools::Panel {
 public:
  explicit SceneHierarchyPanel(HierarchyPanelModel model);
  void render(engine::tools::PanelRenderContext& ctx) override;
  void set_selection(entt::entity entity_id);
 private:
  void render_node(entt::entity entity_id);
  HierarchyPanelModel model_;
};
```

### Edge Cases & Failure Modes

- **Scenes with thousands of entities:** Implement lazy expansion and caching to avoid rebuilding the full tree every frame.
- **Missing name components:** Provide fallback labels (entity IDs) and flag missing metadata for follow-up fixes.
- **Cyclic hierarchies:** Highlight validation warnings using results from the runtime diagnostics pipeline.
- **Detached runtime:** Disable interaction gracefully when the runtime host is paused or disconnected from the editor.

### Test Plan

- **Unit Tests:**
  - Verify the hierarchy walker produces deterministic ordering for parent/child relationships.
  - Ensure selection synchronisation survives entity deletion and re-creation (generational handles).
  - Validate that the panel registers/unregisters with `PanelRegistry` using RAII helpers.
- **Integration Tests:**
  - Editor harness smoke test loads a mock scene and renders the panel with a known entity layout.
  - Runtime loop integration ensures selection events propagate to the scene module.
- **Regression Tests:**
  - Guard against crashes when hierarchy validation reports cycles or missing parents.
  - Confirm the panel hides itself when the tools feature flag disables editor builds.

### Tool Integration

**Diagnostic UI:**
- [ ] Use `render_diagnostics()` to surface validation summaries for the active selection.
- [ ] Register the panel with `PanelRegistry` during TL-310 editor initialization.
- [ ] Emit `PROFILE_SCOPE("SceneHierarchyPanel")` around expensive traversals to preserve telemetry visibility.

**Configuration Management:**
- [ ] Load mock scene configurations for tests via the harness JSON summaries to keep fixtures aligned with AI-004 workflows.

**References:**
- `hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md` — tool adoption guidance.
- `docs/modules/tools/README.md` — ImGui helper usage and panel registry policies.

---

## Steps

1. [ ] Review TL-310 implementation plan and confirm panel registry extension points.
2. [ ] Implement hierarchy model helpers and panel rendering code.
3. [ ] Add editor harness smoke test exercising selection and validation display.
4. [ ] Update tools and scene module READMEs with usage patterns and screenshots.
5. [ ] Capture demo outputs for PM-510 weekly integration cadence.
6. [ ] Land documentation/backlog updates and advance task status.

---

## Evidence

### Test Results

```bash
# Pending — populate once panel implementation lands
# cmake --preset linux-gcc-debug
# cmake --build --preset linux-gcc-debug --target tools_editor
# ctest --preset linux-gcc-debug --tests-regex tools_editor
# pytest scripts/tests/test_dashboard.py
# python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: [pending]
- Integration tests: [pending]
- Documentation validation: [pending]

### Performance

**Benchmark:** Editor UI frame time (mock scene, 5k entities)
- Before: [baseline pending]
- After: [pending]
- Delta: [pending]
