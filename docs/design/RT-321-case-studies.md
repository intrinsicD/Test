# RT-321 Case Study Baselines

**Status:** ✅ Published &amp; maintained alongside packaged manifests

**Related Backlog Items:** `RT-321`, `RT-320`, `TL-210`, `CC-310`, `CC-311`

---

## Overview

The AI-004 harness ships two declarative case studies that exercise remeshing
and rendering workflows end to end. Each scenario binds a curated dataset to the
shared configuration schema, configures the research rendering baseline, and
emits telemetry compatible with comparative benchmarking. This document records
the canonical parameters so runtime, tools, and automation integrations stay in
lock-step when manifests evolve.

---

## Quick Reference

| Case Study | Dataset | Shading | Resolution | Telemetry Output | CTest Target |
|------------|---------|---------|------------|------------------|--------------|
| `geometry-baseline` | `geometry-remesh-baseline` | Deferred | 1280×720 | `telemetry/geometry-baseline/{scenario}.json` | `runtime_prototype_harness_geometry_case_study` |
| `rendering-debug` | `rendering-light-volume` | Forward (overlays enabled) | 1920×1080 | `telemetry/rendering-debug/{scenario}.json` | `runtime_prototype_harness_rendering_case_study` |

Both scenarios can be listed or executed via the harness CLI:

```bash
python -m scripts.prototyping.run_prototype_harness --list-case-studies
python -m scripts.prototyping.run_prototype_harness --case-study geometry-baseline --dry-run --summary-json telemetry/geometry.json
python -m scripts.prototyping.run_prototype_harness --case-study rendering-debug --dry-run --summary-json telemetry/rendering.json
```

The dry-run summaries feed CI artefacts under `out/build/<preset>/artifacts/ai004/`.
Drop `--dry-run` once `libengine_runtime` is available locally to capture
performance metrics.

---

## Geometry Baseline

- **Dataset:** `geometry-remesh-baseline`
  - Input mesh: 4 vertices / 2 faces, surface area 1.0, edge lengths (min 1.0,
    mean 1.1381, max 1.4142).
  - Output mesh: 5 vertices / 4 faces, surface area 1.0, edge lengths (min 0.5,
    mean 0.7906, max 1.118).
  - Remeshing statistics: uniform mode, iterations 6, splits 2, collapses 1,
    duration 3.2 ms.
  - Quality metrics: triangle quality min 0.64 / mean 0.82 / max 0.99; surface
    deviation mean 0.0005, RMS 0.0006.
  - Parameterisation: reuse existing charts, texel density 256, single chart
    covering the unit square with fill ratio 0.95.
- **Rendering:** research baseline preset, deferred shading, 1280×720
  resolution, overlays disabled.
- **Runtime:** orbit camera positioned at (0, 0, 4) targeting the origin,
  timestep 1/60 s with two substeps, hot reload disabled.
- **Telemetry:** file sink at
  `telemetry/geometry-baseline/{scenario}.json`.

---

## Rendering Debug Overlay

- **Dataset:** `rendering-light-volume`
  - Shares the remeshing provenance with the geometry baseline but tunes feature
    preservation for lighting validation (feature edges locked, minimum feature
    angle 35°).
  - Output mesh statistics mirror the geometry baseline (5 vertices / 4 faces,
    edge length mean 0.7906, max 1.118) while remeshing iterations increase to 7
    to accommodate overlay sampling. Surface deviation mean 0.0006, RMS 0.0007.
  - Parameterisation: reuse existing UVs with average stretch 1.01, max stretch
    1.08, fill ratio 0.93.
- **Rendering:** research baseline preset, forward shading, 1920×1080
  resolution. Normals, UV, material, and light-volume overlays default to
  enabled for diagnostics walkthroughs.
- **Runtime:** orbit camera positioned at (1.5, 1.25, 3.5) targeting the origin,
  timestep 1/60 s with two substeps, hot reload polling every 0.5 s.
- **Telemetry:** file sink at
  `telemetry/rendering-debug/{scenario}.json`.

---

## Validation Checklist

- [x] Harness CLI dry-run executed for both scenarios (`runtime_prototype_harness_*` CTest targets).
- [x] Dataset manifests and baseline metrics recorded in this document.
- [ ] Performance baselines captured without `--dry-run` once native runtime
      builds are available (update `average_tick_ms` figures when collected).
- [x] Comparative benchmark report published under
      [`assets/benchmarks/ai004/reports/comparative_report.html`](../../assets/benchmarks/ai004/reports/comparative_report.html)
      with supporting JSON/CSV/SVG artefacts for both case studies.

---

## Update Procedure

1. Regenerate dataset manifests under `assets/datasets/case_studies/` as needed.
2. Re-run the harness CLI for each case study (dry-run for schema validation,
   headless execution for performance numbers).
3. Update the metrics above and adjust the quick-reference table if telemetry
   targets or resolutions change.
4. Run `python scripts/validate_docs.py` before committing documentation edits.
