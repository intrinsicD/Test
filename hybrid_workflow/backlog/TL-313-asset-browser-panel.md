---
id: TL-313
title: Asset browser panel for editor workflows
status: new
priority: P2
area: tools
size: M
owner: tools-lead
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on: ["TL-310 editor foundations"]
links:
  - "hybrid_workflow/backlog/TL-310-editor-foundations.md"
  - "hybrid_workflow/backlog/archive/TOOLS_USAGE_ANALYSIS.md"
  - "docs/modules/tools/README.md"
  - "docs/modules/assets/README.md"
  - "docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md"
---

# Task TL-313 — Asset Browser Panel for Editor Workflows

## Intent

Deliver an asset browser panel that exposes cache contents, hot-reload status, and metadata so content authors can inspect resources directly from the editor while running hybrid workflow demos.

---

## Context

**Current State:**
- Asset caches support generational handles, hot reload, and telemetry, but the editor lacks a unified UI for browsing them.
- TL-310 re-enables panel infrastructure without bundling domain-specific panels.
- [`TOOLS_USAGE_ANALYSIS.md`](archive/TOOLS_USAGE_ANALYSIS.md) identifies the missing asset browser as a follow-up required to leverage tooling fully.

**Desired State:**
- The editor lists meshes, textures, materials, and shaders with search/filter controls and displays live reload status.
- Asset metadata (source path, checksum, GPU residency, dependency graph) is surfaced for diagnostics and dataset validation.
- Weekly demos (PM-510) can demonstrate asset health without switching to CLI tooling.

**References:**
- [`docs/modules/assets/README.md`](../../docs/modules/assets/README.md) — cache behaviour, hot reload policies, and telemetry coverage.
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — ImGui helper usage and tooling invariants.
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) — runtime/tooling contract for resource ownership.
- [`hybrid_workflow/backlog/TL-310-editor-foundations.md`](TL-310-editor-foundations.md) — editor enablement baseline.
- [`hybrid_workflow/backlog/archive/TOOLS_USAGE_ANALYSIS.md`](archive/TOOLS_USAGE_ANALYSIS.md) — recommendation to add TL-313 asset browser follow-up.

---

## Design / Plan

### Constraints

- Query asset caches through existing APIs; do not bypass ownership/locking guarantees.
- Reflect hot reload state changes immediately using the asynchronous notifications already exposed by the assets module.
- Provide filters that scale to hundreds of assets without compromising UI responsiveness.
- Respect the tools feature flag and degrade gracefully in builds without asset streaming enabled.
- Synchronise doc/backlog updates so Bundle B status stays current.

### API / Data Sketch

```cpp
struct AssetDescriptorRow {
  std::string name;
  engine::assets::AssetType type;
  std::filesystem::path source_path;
  std::string checksum;
  engine::assets::AssetState state;
};

class AssetBrowserPanel : public engine::tools::Panel {
 public:
  explicit AssetBrowserPanel(AssetRegistryFacade facade);
  void render(engine::tools::PanelRenderContext& ctx) override;
 private:
  void render_toolbar();
  void render_asset_table();
  void render_details(const AssetDescriptorRow& row);
  std::vector<AssetDescriptorRow> rows_;
  std::string filter_text_;
};
```

### Edge Cases & Failure Modes

- **Large datasets:** Implement incremental pagination or virtualised tables to avoid UI stalls.
- **Hot reload failures:** Surface failure reasons and link to diagnostics/telemetry guidance.
- **Missing metadata:** Provide placeholders and mark entries requiring dataset regeneration.
- **Headless configurations:** Hide GPU residency columns when running without graphics backends.

### Test Plan

- **Unit Tests:**
  - Validate asset descriptor aggregation handles mixed asset types.
  - Ensure filtering logic is case-insensitive and stable.
  - Confirm panel registration/unregistration semantics with the registry.
- **Integration Tests:**
  - Editor harness smoke test loads sample datasets and verifies the panel lists expected assets.
  - Hot reload simulation toggles asset state and checks UI updates.
- **Regression Tests:**
  - Guard against crashes when new asset types are added.
  - Snapshot test ensures table columns remain aligned with documentation.

### Tool Integration

**Diagnostic UI:**
- [ ] Register panel via `PanelRegistry` and reuse ImGui helper components.
- [ ] Integrate `render_diagnostics()` or equivalent to highlight failing reloads.

**Configuration Management:**
- [ ] Use harness configuration JSON to populate asset roots for smoke tests.

**Benchmark Automation:**
- [ ] Optionally hook into benchmark runners to mark assets participating in current scenarios.

---

## Steps

1. [ ] Audit asset cache APIs and hot reload notification hooks.
2. [ ] Implement descriptor aggregation and panel rendering.
3. [ ] Extend editor harness smoke test with sample assets and hot reload toggles.
4. [ ] Document usage in tools and assets READMEs.
5. [ ] Capture PM-510 demo artefacts showing asset diagnostics.
6. [ ] Update backlog/roadmap status and land the change.

---

## Evidence

### Test Results

```bash
# Pending — populate when panel ships
# cmake --preset linux-gcc-debug
# cmake --build --preset linux-gcc-debug --target tools_editor
# ctest --preset linux-gcc-debug --tests-regex tools_editor_assets
# python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: [pending]
- Integration tests: [pending]
- Documentation validation: [pending]

### Performance

**Benchmark:** Asset table refresh latency (1k entries)
- Before: [pending]
- After: [pending]
- Delta: [pending]
