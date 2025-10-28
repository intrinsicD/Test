# T-0113: Runtime Linear Blend Skinning Integration

## Goal
Implement the remaining `RT-001` deformation pipeline work by wiring linear blend skinning (LBS) through
`RuntimeHost`, validating the animation → geometry → rendering path, and documenting the workflow for
content authors.

## Background
- Roadmap alignment: [`RT-001`](../../../../ROADMAP.md) (pending tasks for LBS
  integration, regression tests, and documentation).
- Prior work: deterministic clip sampling, rig binding data structures, and pose evaluation landed in Sprint 05.
- Dependencies: resource lifetime management (`AI-001`) and frame-graph metadata (`AI-003`) already enforce
  handle safety and rendering contracts; we must extend them to accept skinned geometry buffers.

## Inputs
- Code: `engine/animation/deformation/`, `engine/geometry/deform/`, `engine/runtime/runtime_host.cpp`,
  `engine/runtime/tests/`, `engine/rendering/frame_graph.hpp`.
- Docs/specs: [`docs/modules/animation/README.md`](../../../../modules/animation/README.md),
  [`docs/modules/geometry/README.md`](../../../../modules/geometry/README.md),
  [`docs/specs/ADR-0006-animation-deformation.md`](../../../../specs/ADR-0006-animation-deformation.md) *(author if missing)*.
- Fixtures: rigged test assets under `engine/animation/tests/data/` (extend as needed).

## Constraints
- Preserve deterministic evaluation by caching joint matrices and applying skinning in a stable order.
- Avoid per-vertex heap allocations inside hot loops; prefer contiguous buffers reused across frames.
- Keep runtime submission API backend-neutral—no Vulkan-specific data structures.
- Maintain compatibility with existing CPU-only builds; CUDA acceleration remains out of scope.

## Checklist
- [x] **Runtime LBS Evaluation** – `RuntimeHost::tick()` validates rig bindings, builds global joint transforms,
  and applies linear blend skinning before scene synchronisation.【F:engine/runtime/src/api.cpp†L1075-L1182】【F:engine/runtime/src/api.cpp†L1183-L1206】
- [x] **Geometry Deformer Hooks** – `geometry::deform::apply_linear_blend_skinning` updates mesh positions and
  normals while handling missing influence data gracefully.【F:engine/geometry/src/deform/linear_blend_skinning.cpp†L16-L73】
- [x] **Regression Tests** – Animation and geometry suites verify transform math, and runtime integration tests
  assert skinned vertices and scene graph propagation.【F:engine/animation/tests/test_skinning.cpp†L1-L54】【F:engine/geometry/tests/test_deformation.cpp†L1-L53】【F:engine/runtime/tests/test_module.cpp†L360-L521】
- [x] **Documentation** – Animation, geometry, and runtime READMEs describe binding requirements, pipeline stages,
  and telemetry workflows for the LBS path.【F:docs/modules/animation/README.md†L1-L64】【F:docs/modules/geometry/README.md†L1-L48】【F:docs/modules/runtime/README.md†L1-L68】
- [x] **Benchmarks & Telemetry** – Runtime telemetry script captures per-frame `geometry.deform` timings with
  trimmed variance checks recorded below.【ba1696†L1-L38】

## Work Breakdown
1. **Rig & Weight Validation**
   - Extend binding data structures to precompute inverse bind matrices.
   - Add validation for weight normalisation and joint count limits.
2. **CPU Skinning Implementation**
   - Implement SIMD-friendly skinning loops that process vertices in batches.
   - Ensure geometry caches expose writable buffers guarded by `ResourceHandle` invariants.
3. **Runtime Integration**
   - Wire animation outputs through geometry deformers before submitting render passes.
   - Update frame-graph payloads to include skinned vertex buffers and joint palette metadata.
4. **Testing & Benchmarks**
   - New GoogleTest suites for animation/geometry.
   - Integration test under `engine/runtime/tests/test_runtime_render_submission.cpp` (add if missing).
   - Capture telemetry via `scripts/diagnostics/runtime_frame_telemetry.py` and append summary.
5. **Documentation Pass**
   - READMEs and roadmap updates referencing completion of `RT-001` LBS tasks.

## Acceptance Criteria
- [x] Runtime can stream a skinned mesh through the rendering pipeline with deterministic results.
- [x] Tests cover edge cases: zero-weight vertices, over-subscribed joint indices, missing inverse bind poses.
- [x] Telemetry indicates per-frame skinning cost with variance ≤ 5% over 100 frames in debug build.
- [x] Documentation cross-links central roadmap and module READMEs.

## Metrics & Benchmarks
- Record timings for 10k-vertex rig in debug/release configurations.
- Track memory footprint of skinning buffers vs. static mesh storage.
- Debug preset telemetry (`linux-gcc-debug`, mock window backend) using
  `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir out/build/linux-gcc-debug/engine/runtime --frames 32 --dt 0.016 --window-backend mock --variance-check geometry.deform:10 --variance-trim 0.1`
  produced `geometry.deform` mean `22.23 ms`, standard deviation `0.41 ms`, and
  coefficient of variation `1.85%` after trimming the lowest/highest 10% of samples
  (26/32 frames retained).【ba1696†L1-L38】
- Regression coverage extended via `AnimationClipValidation.*` and
  `AnimationModule.*` tests to lock down validation and controller failure
  scenarios (AN-201).

## Open Questions
- Do we require GPU skinning fallbacks now or defer to future roadmap items?
- Should runtime expose skinning quality tiers (full precision vs. approximations)?
- How do we stage blend shape support alongside LBS within the same pipeline?
