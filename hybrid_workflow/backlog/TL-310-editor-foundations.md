---
id: TL-310
title: Editor foundations & tooling enablement
status: in_progress
priority: P2
area: tools
size: L
owner: tools-lead
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on: ["RT-410"]
links: ["docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md", "docs/modules/tools/README.md", "hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md", "hybrid_workflow/backlog/PM-510-weekly-integration-demos.md"]
---

# Task TL-310 — Editor Foundations & Tooling Enablement

## Intent

Re-enable the tools/editor module with shared panel registries and runtime integration so GPU-backed demos run inside the editor once runtime presentation hooks are ready.

---

## Context

**Current State:**
- Tools module targets remain disabled in CMake presets.
- Panel registry implementation lives only in documentation and samples.
- Weekly demos highlight tooling gaps while runtime/presentation work lands.

**Desired State:**
- Editor builds compile by default with feature flags for experimental panels.
- Panel registry and sandbox hooks bridge runtime diagnostics to ImGui overlays.
- CI smoke tests and documentation describe editor setup for PM-510 demos.

**References:**
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [`docs/modules/tools/README.md`](../docs/modules/tools/README.md)
- [`hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md`](archive/RT-410-runtime-stage-planner.md)
- [`hybrid_workflow/backlog/PM-510-weekly-integration-demos.md`](PM-510-weekly-integration-demos.md)

### Context Ladder Notes — 2025-11-04

- [`README.md`](../../README.md) confirms the tools module remains disabled and highlights TL-310 as the vehicle for restoring editor builds once runtime presentation hooks arrive, reinforcing the dependency recorded in `blocked_on`.
- [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) directs agents to the hybrid workflow artefacts and backlog before touching code, so this task will continue logging status, evidence, and coordination in sync with those references.
- [`docs/ROADMAP.md`](../../docs/ROADMAP.md) lists TL-310 within Bundle B (Priority 2) and sequences it behind RT-410, so current work focuses on planning and documentation alignment while runtime adapters mature.
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) reiterates that ADR-0008 integration and editor build re-enablement are outstanding, providing the module-level invariants that the implementation must satisfy.

---

## Design / Plan

### Constraints

- Respect runtime presentation adapters delivered by RT-410; avoid duplicate swap chains.
- Keep ImGui overlays deterministic with existing diagnostics infrastructure.
- Ensure build presets gate optional dependencies with clear documentation.
- Restore CI smoke coverage without prolonging runtime for unrelated modules.

### API / Data Sketch

```cpp
namespace engine::tools {

struct PanelRegistration {
  std::string_view identifier;
  PanelFactory factory;
  PanelLifecycleHooks lifecycle;
};

class PanelRegistry {
public:
  void RegisterPanel(PanelRegistration registration);
  void ForEachPanel(const PanelVisitor& visitor) const;
};

} // namespace engine::tools
```

### Edge Cases & Failure Modes

- **Missing runtime hooks:** Feature-flag editor startup until RT-410 surfaces presentation adapters.
- **Duplicate GPU contexts:** Share GPU resource handles with rendering backends to prevent leaks.
- **Scripted panel crashes:** Harden registry against exceptions and document error handling.
- **CI instability:** Provide headless-safe presets to exercise editor smoke tests without graphics output.

### Test Plan

- **Unit Tests:**
  - Panel registry registration/lookup lifecycle.
  - Sandbox configuration loader for editor harness.
- **Integration Tests:**
  - Editor harness boots with mock backend in CI.
  - PM-510 demo scenario records editor overlays once GPU path active.
- **Regression:**
  - Validate feature flag toggles for enabling/disabling editor builds.

---

## Steps

1. [x] Update CMake presets to re-enable tools module behind feature flag.
   - (2025-11-06) Added `ENGINE_ENABLE_TOOLS` cache entry with presets defaulting to `ON` so the tools library/tests build while remaining easy to disable for lean configurations.
2. [ ] Implement panel registry and editor harness bridge in `engine/tools/src/`.
3. [ ] Restore unit tests in `engine/tools/tests/` for registry + configuration loader.
4. [ ] Add editor smoke scenario to scripts/tests harness.
5. [ ] Refresh tools README and root README to describe revived workflow.
6. [ ] Coordinate with PM-510 to schedule demo once runtime hooks ready.
7. [ ] Capture test outputs, update docs, and advance task status.
8. [ ] Transition panel-specific work to follow-up tasks (`TL-311`–`TL-314`) once the registry bridge stabilises.
   - Ensure each panel task inherits context and validation hooks from this baseline implementation.

**Status Update (2025-11-04):** Initiated hybrid workflow for TL-310 by assembling context notes and aligning roadmap/backlog status; implementation will begin once RT-410 exposes presentation adapters required for editor bring-up.

---

## Evidence

### Test Results

```bash
# Pending — populate when editor enablement patches land
```

**Test Summary:**
- Unit tests: pending implementation
- Integration tests: pending implementation
- Documentation validation: pending implementation

**Notes:** Planning-only phase; no code, tests, or benchmarks executed while RT-410 dependency remains outstanding.

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [ ] Pending | QA/Test | Editor unit + smoke tests |
| docs | [ ] Pending | Docs/DevRel | Tools README, navigation, roadmap updates |
| perf | [ ] N/A | — | — |
| safety | [ ] N/A | — | — |
| release | [ ] Pending | Release Mgr | Feature flag + rollout notes |

### Updated Files

- `engine/tools/CMakeLists.txt`
- `engine/tools/include/engine/tools/imgui/panel_registry.hpp`
- `engine/tools/src/imgui/panel_registry.cpp`
- `engine/tools/tests/test_panel_registry.cpp`
- `scripts/tests/test_editor_smoke.py`
- `docs/modules/tools/README.md`
- `README.md`
- `docs/ROADMAP.md`

---

## Completion Checklist (Definition of Done)

- [ ] Tools module builds enabled with clear feature flag documentation.
- [ ] Panel registry, sandbox bridge, and editor harness implemented with tests.
- [ ] CI smoke coverage restored for editor scenarios.
- [ ] Documentation updated (tools README, root README, roadmap, navigation if needed).
- [ ] PM-510 demo captures editor overlays once runtime hooks exist.
- [ ] Status moved to `done` and task archived after sign-offs.

---

## Result

**PR:** (pending completion)

**SHA:** (pending merge)

**Completion Date:** (sequenced)

**Notes:**
- Stage work to land immediately after RT-410 marks status ready.
- Align documentation updates with Docs/DevRel to avoid conflicting messaging.
- Evaluate follow-up tasks for plugin sandboxing once baseline editor stable.

**Follow-ups:**
- [ ] Investigate plugin sandbox hardening → create TL-311.
- [ ] Document editor theming pipeline → create docs task.

---

## Role Coordination

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Agent Orchestrator | Agent Orchestrator | Align tooling activation with runtime GPU milestones | Sequenced |
| Product Manager | Product Manager | Prioritise tooling backlog relative to runtime deliverables | Sequenced |
| Knowledge Librarian | Knowledge Librarian | Capture documentation updates + tutorials | Queued |
| Specialist Engineer(s) | Tools Lead | Re-enable builds, implement registries, integrate harness | Ready |
| Docs/DevRel | Docs Team | Update docs/modules/tools and tutorials | Queued |
| QA/Test Specialist | QA Lead | Restore editor smoke coverage | Queued |
| Performance Engineer | Performance Lead | Profile editor overlays post-enablement | Planned |
| Safety Reviewer | Security Reviewer | Review plugin loading + sandboxing plan | Planned |
| Reviewer | Tools Reviewer | Provide code review | Sequenced |
| Release Manager | Release Manager | Manage feature flag and communication plan | Sequenced |
