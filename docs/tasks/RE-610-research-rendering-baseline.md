# Task Card: RE-610

## Title
Research Rendering Baseline

## Type
- [x] Feature
- [ ] Bug Fix
- [x] Refactor
- [ ] Documentation
- [x] Research
- [x] Performance Optimization

## Priority
- [x] Critical (P0)
- [ ] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
3 weeks (rendering + runtime integration)

---

## Description

### Problem Statement
Researchers need a ready-to-use rendering configuration that mirrors modern physically based pipelines, exposes debug visualisations, and integrates telemetry for benchmarking. Current rendering samples focus on subsystem validation and do not package lighting models, post-processing, or shader hot reload suitable for rapid experiments.

### Proposed Solution
Create a curated frame graph preset that includes deferred and forward shading variants, screen-space debugging tools, and instrumentation hooks. Ship precompiled shader packages, document hot reload workflows, and expose toggles through the prototyping harness. Integrate GPU timing, draw call analytics, and shading variant metadata into the telemetry schema consumed by benchmarking scripts.

### Success Criteria
- Default rendering preset renders provided research scenes with PBR lighting, shadow mapping, and tone mapping at ≥ 120 FPS on target hardware.
- Debug overlays (normals, wireframe, material properties) can be toggled without recompiling the engine.
- Telemetry exports capture shading variant metadata to support benchmark comparisons.
- Documentation enables researchers to add new shader variants within one iteration.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::rendering`
- `engine::runtime`
- `engine::assets`
- `engine::tools`

**Files to Modify:**
- `engine/rendering/include/...` frame graph presets
- `engine/rendering/src/...` pipeline implementations
- `engine/rendering/tests/`
- `docs/modules/rendering/README.md`
- `docs/ROADMAP.md`
- `python/scripts/` telemetry exporters

**New Files:**
- `engine/rendering/presets/research_baseline.hpp/.cpp`
- `assets/shaders/research_baseline/*`
- `docs/design/RE-610-research-pipeline.md`

### Dependencies
**Depends On:**
- `AI-001` handle validation (for shader hot reload)
- `AI-003` frame-graph metadata (complete)

**Blocks:**
- `RT-320` (needs baseline pipeline)
- `TL-210` (UI toggles reference preset)
- Benchmark automation in `CC-310`

### Related Work
- `T-0121-rendering-standard-passes-library.md`
- `T-0123-rendering-pipeline-state-management.md`
- `docs/specs/ADR-0003-runtime-frame-graph.md`

---

## Acceptance Criteria

### Functional Requirements
- [ ] Frame graph preset with configurable forward and deferred shading paths.
- [ ] Shader hot reload supported for baseline pipeline with validation messages.
- [ ] Debug overlays for normals, UV density, material parameters, and light volumes.
- [ ] Telemetry counters for draw calls, GPU time per pass, and shading variant selection exported via diagnostics.

### Non-Functional Requirements
- [ ] Performance: Baseline achieves ≥ 120 FPS on RTX 3070 @ 1080p in sample scene.
- [ ] Memory: GPU memory footprint ≤ 4 GB for reference scene.
- [ ] Latency: Shader hot reload turnaround ≤ 1 second.

### Testing Requirements
- [ ] Unit tests for preset creation and pipeline state validation.
- [ ] Integration test renders sample scene and validates telemetry output.
- [ ] Golden screenshot comparison for baseline vs. debug overlays.
- [ ] Coverage ≥ 85% on new rendering code.

### Documentation Requirements
- [ ] Update rendering README with preset usage guide.
- [ ] Add shader authoring guide referencing dataset assets.
- [ ] Document telemetry fields in `design/TELEMETRY_SCHEMA.md` appendix.
- [ ] Publish quickstart tutorial under `docs/design/` for research baseline.

---

## Test Plan

### Unit Tests
```cpp
TEST(RenderingResearchBaseline, CreatesDeferredPreset) {
    auto preset = rendering::CreateResearchBaselinePreset(RenderingMode::kDeferred);
    EXPECT_TRUE(preset.is_valid());
    EXPECT_EQ(preset.pass_count(), expected_passes);
}
```

### Integration Tests
- Launch runtime sample with research preset, render 120 frames, and verify telemetry counters.
- Validate shader hot reload pipeline by editing sample shader and asserting updated frame graph state.

### Performance Tests
- Capture GPU timings for baseline passes using Tracy/Tracy GPU zones and ensure regression thresholds enforced in CI.

---

## Implementation Notes

### Design Considerations
- Use frame graph descriptors to expose parameterized attachments for experiments.
- Provide fallback pipelines for non-ray-tracing hardware while preserving comparable outputs.
- Align shader packaging with assets module to reuse hot reload plumbing.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Shader hot reload destabilises runtime | Medium | High | Add validation layers and fallback to previous shader on failure.
| GPU performance varies widely | Medium | Medium | Document baseline hardware; capture scaling guidance for alternate GPUs.
| Asset pipeline mismatch | Low | Medium | Version shader/material definitions with dataset packages (`AS-330`).

### Alternative Approaches
1. **Minimal forward-only pipeline** → rejected; insufficient for complex research workloads.
2. **External shader pack** → rejected; would fragment telemetry integration and complicate updates.

---

## Deliverables

- [ ] Rendering preset implementation + tests
- [ ] Shader pack + material definitions
- [ ] Telemetry instrumentation + documentation
- [ ] Updated docs + tutorials
- [ ] Performance baseline report
- [ ] Linked PRs referencing `RE-610`

---

## Definition of Done

- [ ] CI builds/tests green on all presets
- [ ] Rendering benchmarks capture expected metrics
- [ ] Documentation reviewed by rendering and research leads
- [ ] Telemetry viewer surfaces new counters
- [ ] Runtime harness integrates preset by default

---

## Assigned To
**Role**: Rendering Engineer
**Name**: @rendering-lead

## Estimated Timeline
**Start Date**: 2025-11-25
**Target Completion**: 2025-12-16
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with tools team for UI toggles.
- Provide fallback lighting configuration for laptops with limited GPU capacity.
