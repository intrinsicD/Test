# Animation Module

## Current State

- Provides clip, track, and controller primitives with sampling implemented in `src/api.cpp`.
- Supplies a deterministic oscillator clip that powers runtime smoke tests and demonstrations.
- Validates clips for structural issues and serialises/deserialises them as JSON to integrate with external asset tooling.
- Exposes blend-tree authoring helpers (clip nodes, linear blends, tree evaluation/advancement) so runtime clients can compose multiple motion sources.
- Blend trees support named float, bool, and event parameters that can drive linear blend weights and other runtime controls.
- Public headers expose the core data structures consumed by other subsystems; implementation is kept ABI-stable for external tooling.

## Usage

- Link against `engine_animation` and include `<engine/animation/api.hpp>`; the target inherits the shared `engine::project_options` compile features and contributes its headers to `engine::headers`.
- Use `write_clip_json`/`read_clip_json` (or the filesystem helpers) to persist clips and `validate_clip` before exporting author-authored data.
- Build blend trees by creating `AnimationBlendTree` instances, adding clip/controller nodes, wiring blend nodes, and calling `evaluate_blend_tree` each frame.
- Extend controllers or clips under `src/` and update accompanying tests in `tests/`.
- Build and test the module with the canonical presets (for example `cmake --preset linux-gcc-debug` followed by `ctest --preset linux-gcc-debug`).

## Roadmap

See [docs/roadmaps/animation.md](../../docs/roadmaps/animation.md) for the authoritative milestone plan and
[global alignment overview](../../docs/global_roadmap.md) for cross-module dependencies.

## TODO / Next Steps

### Immediate (M3 – Due 2025-11-15)
- [ ] Implement additive blend nodes (@animation-team, #234)
  - Support additive pose composition
  - Add tests for additive blending
- [ ] Fix quaternion normalization edge case (@alice, #235)
  - Normalize after blend to prevent drift
  - Add regression test

### Short-term (M4 – Due 2025-12-30)
- [ ] Deformation integration (@bob, #236)
  - Define rig binding data structures
  - Implement linear blend skinning
  - Surface GPU pose buffers
- [ ] Performance profiling (@carol, #237)
  - Benchmark blend tree evaluation
  - Profile memory allocations
  - Document performance characteristics

### Mid-term (M5–M6)
- [ ] Editor tooling for blend trees
- [ ] State machine nodes
- [ ] Dual quaternion skinning

## Test Coverage

### Current Coverage (M3)
- ✅ Module identity
- ✅ Controller advancement and evaluation
- ✅ Linear blend tree evaluation
- ✅ Parameter binding (float, bool, event)
- ✅ Clip validation errors
- ✅ JSON serialization round-trip

### Missing Coverage (Tracked in #238)
- [ ] Non-looping playback edge cases
- [ ] Blend tree with >10 nodes (stress test)
- [ ] Invalid parameter binding (type mismatch)
- [ ] Quaternion renormalization after many blends
- [ ] Empty track handling in sampling
- [ ] Clip duration edge cases (zero duration, negative)

### Coverage Targets
- **Current:** ~85% line coverage
- **Target:** 90% by M4
- **Tool:** Run `ctest --coverage` to generate report

## Performance

### Characteristics
- **Thread Safety:** Read-only operations are thread-safe
- **Memory:** Blend tree evaluation allocates temporary pose cache
- **Scalability:** $\mathcal{O}(n)$ where $n$ is the number of rig joints

### Benchmarks (M3)
- Single blend tree evaluation: ~50µs (100 joints, 10 nodes)
- Clip sampling: ~5µs per joint
- Parameter updates: $\mathcal{O}(1)$

### Known Bottlenecks
- Quaternion slerp in hot path (tracked in #239)
- Pose map allocations during blend (tracked in #240)
