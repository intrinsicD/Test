# Context Package — TL-313 Asset Browser Panel

## 1. Source Documents & Highlights

| # | Reference | Key Notes | Follow-ups |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Confirms hybrid workflow emphasis, module directory layout, and tooling expectations for ImGui panels. | None |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Tools and assets module docs listed under Engine Modules → ensures new sections must be reflected if we add subsections. | Update if new subsection added in docs. |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Bundle B highlights tooling panels for editor readiness; TL-313 is next action post TL-310 registry work. | After landing, update bundle status if needed. |
| 4 | [`hybrid_workflow/backlog/TL-313-asset-browser-panel.md`](../../hybrid_workflow/backlog/TL-313-asset-browser-panel.md) | Acceptance criteria emphasise cache metadata, filtering, docs/tests alignment, and Steps log updates. | Mark steps + status transitions during implementation. |
| 5 | [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) | Documents `PanelRegistry`, Dear ImGui conventions, profiling instrumentation expectations. | Extend to mention asset browser usage and diagnostics. |
| 6 | [`docs/modules/assets/README.md`](../../docs/modules/assets/README.md) | Details cache lifecycle mixin, telemetry, handle validation. Informs safe introspection design. | Document new `for_each_asset` helper and tooling integration. |
| 7 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Defines runtime/editor contract, requiring panel registration via bridge and feature-flag-aware behaviour. | Ensure bridge hooks obey ADR; mention asset panel registration. |
| 8 | [`docs/reviews/2025-03-22-SCENE-DOCS.MD`](../../docs/reviews/2025-03-22-SCENE-DOCS.MD) | Scene panel lessons: deterministic ordering, avoid UI stalls, record validation behaviour. | Apply to asset panel (sorting, ImGuiListClipper). |

## 2. Open Questions & Resolutions
- **How to expose cache contents without breaking encapsulation?** → Extend `AssetCacheLifecycle` with read-only iteration helper; document in module README.
- **What metadata is readily available?** → Mesh/graph caches carry detection info; textures expose dimensions/mip levels; shader/material descriptors provide stage/handle references. No GPU residency yet.
- **How to maintain UI responsiveness for large caches?** → Use `ImGuiListClipper`, incremental filtering, and maintain cached lower-case query.

## 3. Implementation Notes
- Panel should order assets by type then identifier for determinism.
- Filtering must match identifier, source path, status, and metadata values.
- Bridge integration should respect `ENGINE_ENABLE_ASSETS` and skip registration when caches unavailable.
- Provide helper `collect_asset_rows(facade)` so runtime wiring is straightforward.

## 4. Risks & Mitigations
- **Risk:** Frequent allocations when refreshing rows each frame. → Use move semantics, reserve vector capacity, and only recompute sort when dataset changes.
- **Risk:** Absent ImGui context (headless validation). → Guard render path using `ImGui::GetCurrentContext()` like existing panels.
- **Risk:** Hot reload watchers causing file handles to leak in tests. → Ensure temporary assets cleaned up; rely on cache RAII.

## 5. Coordination Log
- 2026-05-09 — Synced with Docs/DevRel on documentation touchpoints.
- 2026-05-09 — Confirmed QA gate executes full build/test/pytest/doc validation stack.

