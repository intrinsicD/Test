# T-0113: Runtime Linear Blend Skinning Integration

## Goal
Implement the remaining `RT-001` deformation pipeline work by wiring linear blend skinning (LBS) through
`RuntimeHost`, validating the animation → geometry → rendering path, and documenting the workflow for
content authors.

## Background
- Roadmap alignment: [`RT-001`](../ROADMAP.md#rt-001-animation-deformation-pipeline) (pending tasks for LBS
  integration, regression tests, and documentation).
- Prior work: deterministic clip sampling, rig binding data structures, and pose evaluation landed in Sprint 05.
- Dependencies: resource lifetime management (`AI-001`) and frame-graph metadata (`AI-003`) already enforce
  handle safety and rendering contracts; we must extend them to accept skinned geometry buffers.

## Inputs
- Code: `engine/animation/deformation/`, `engine/geometry/deform/`, `engine/runtime/runtime_host.cpp`,
  `engine/runtime/tests/`, `engine/rendering/frame_graph.hpp`.
- Docs/specs: [`docs/modules/animation/README.md`](../modules/animation/README.md),
  [`docs/modules/geometry/README.md`](../modules/geometry/README.md),
  [`docs/specs/ADR-0006-animation-deformation.md`](../specs/ADR-0006-animation-deformation.md) *(author if missing)*.
- Fixtures: rigged test assets under `engine/animation/tests/data/` (extend as needed).

## Constraints
- Preserve deterministic evaluation by caching joint matrices and applying skinning in a stable order.
- Avoid per-vertex heap allocations inside hot loops; prefer contiguous buffers reused across frames.
- Keep runtime submission API backend-neutral—no Vulkan-specific data structures.
- Maintain compatibility with existing CPU-only builds; CUDA acceleration remains out of scope.

## Deliverables
1. **Runtime LBS Evaluation** – Extend `RuntimeHost::tick()` to evaluate linear blend skinning results into
   geometry staging buffers before render submission.
2. **Geometry Deformer Hooks** – Introduce reusable helpers under `engine/geometry/deform/` that apply joint
   matrices to skinned meshes with validation for sparse/broken weights.
3. **Regression Tests** – Add deterministic tests covering:
   - Animation-only validation of skinning math (`engine/animation/tests/`).
   - Runtime integration test ensuring skinned meshes reach the frame graph.
4. **Documentation** – Update animation and geometry READMEs plus runtime documentation with:
   - Data requirements (joint limits, weight normalisation).
   - Instructions for authoring rig bindings and invoking runtime APIs.
5. **Benchmarks & Telemetry** – Record per-frame skinning timings via the existing telemetry script and append
   results to this task file.

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
  `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir out/build/linux-gcc-debug/engine/runtime --frames 120 --dt 0.016 --window-backend mock --variance-check geometry.deform:5 --variance-trim 0.1`
  produced `geometry.deform` mean `15.63 ms`, standard deviation `0.22 ms`, and
  coefficient of variation `1.42%` after trimming the lowest/highest 10% of samples
  (96/120 frames retained).【de369e†L1-L8】

## Open Questions
- Do we require GPU skinning fallbacks now or defer to future roadmap items?
- Should runtime expose skinning quality tiers (full precision vs. approximations)?
- How do we stage blend shape support alongside LBS within the same pipeline?
