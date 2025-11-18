---
id: TL-317
title: Outline rendering for selected entities
status: in_progress
priority: P1
area: rendering
size: M
owner: unassigned
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on:
  - "TL-315"
links:
  - "docs/modules/rendering/README.md"
  - "docs/modules/tools/README.md"
  - "docs/specs/ADR_0005_RENDERING_FRAMEGRAPH.md"
---

# Task TL-317 — Outline Rendering for Selected Entities

## Intent

Render consistent selection outlines (silhouettes or halo) for single and multi-selected entities so tooling panels, viewport overlays, and PM-510 demos can highlight the active context directly in the scene.

---

## Context

**Current State:**
- The viewport does not draw any selection indicators; hierarchy selections rely on ImGui text highlights only.
- Without an outline, debugging geometry or runtime state requires toggling wireframe modes manually, leading to poor UX.
- The rendering module already supports post-process passes and ID buffers but lacks a reusable outline pass wired to the selection engine.

**Desired State:**
- Selected entities emit a stylized outline (color configurable) composited after the main pass without disrupting existing frame-graph nodes.
- Outline rendering can use ID buffers (preferred) or depth-only silhouettes as fallback when selection IDs are unavailable.
- The pass scales from single-entity outlines to multi-selection halos while capping GPU cost (≤0.5 ms budget on reference scene).

**References:**
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) — frame-graph and post-process conventions.
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — tooling overlays + viewport diagnostics.
- [`docs/specs/ADR_0005_RENDERING_FRAMEGRAPH.md`](../../docs/specs/ADR_0005_RENDERING_FRAMEGRAPH.md) — guarantees for adding new passes and resource lifetimes.

---

## Design / Plan

### Constraints

- No extra full-resolution pass unless selection is active; skip work when the selection stack is empty.
- Reuse ID or depth buffers from TL-315 color picking path to avoid duplicating render targets.
- Outline shader must be compatible with both forward and deferred renderers per ADR_0005.
- Provide tooling API to configure color/thickness; defaults align with editor theme.

### API / Data Sketch

```cpp
struct OutlineConfig {
  LinearColor color;
  float thickness_px;
  bool pulse_animation;
};

class SelectionOutlinePass : public FrameGraphPass {
 public:
  SelectionOutlinePass(const OutlineConfig& cfg,
                       FrameGraphResource id_buffer,
                       FrameGraphResource depth_buffer);
  void add_pass(FrameGraph& fg, const SelectionBuffer& selection);
};
```

### Edge Cases & Failure Modes

- **MSAA mismatch:** ensure outline sampling matches color buffer sample count to avoid halos.
- **Thin geometry:** fallback to depth-based sobel detection when ID buffer lacks coverage (e.g., wires, points).
- **High selection counts:** clamp number of simultaneously highlighted entities or batch draws to avoid saturating fill rate.
- **Headless mode:** degrade gracefully (e.g., log) when render targets unavailable.

### Test Plan

- **Unit Tests:**
  - Validate shader constants + outline thickness computation helpers.
  - Ensure frame-graph registration handles missing resources without crashing.
- **Integration Tests:**
  - Geometry viewer screenshot/CI test verifying outlines appear for selected meshes and update when selection changes.
  - GPU test verifying pass short-circuits when no selection exists.
- **Regression Tests:**
  - Guard against color picking + outline conflicts on shared render targets.
  - Verify animation/pulse feature remains deterministic across frames.

### Tool Integration

**Diagnostic UI:**
- [ ] Expose outline color/thickness in the editor settings panel.
- [ ] Provide debug visualization for ID/depth buffers feeding the outline pass.

**Profiling:**
- [ ] Capture GPU timing queries for the outline pass to enforce ≤0.5 ms budget.

---

## Steps

1. [x] Align with TL-315 selection buffer outputs and identify required render targets.
2. [x] Implement outline shader + frame-graph pass supporting ID and depth fallbacks.
3. [x] Integrate pass into viewport pipeline (OpenGL/Vulkan) with dynamic enable/disable.
4. [x] Expose configuration hooks to tools/editor and default theme colors.
5. [x] Add tests + CI screenshots (geometry_viewer) plus documentation updates.
6. [ ] Record profiling evidence and update roadmap/backlog status.

---

## Evidence

- `cmake --preset linux-gcc-debug` (configures toolchain and presets) 【8f7663†L1-L41】
- `cmake --build --preset linux-gcc-debug --target engine_rendering_tests` 【5466e1†L1-L20】
- `ctest --preset linux-gcc-debug -R engine_rendering_tests --output-on-failure` 【e483ae†L1-L9】
