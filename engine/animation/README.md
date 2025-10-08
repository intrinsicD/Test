# Animation

_Path: `engine/animation`_

_Last updated: 2025-03-15_


## Runtime responsibilities

The animation subsystem now ships a small but functional controller stack that powers the runtime smoke test. The
public API under [`include/engine/animation/api.hpp`](include/engine/animation/api.hpp) defines:

- **`AnimationClip` / `JointTrack` / `Keyframe`** – Plain-old-data containers that capture skeletal channels with
  explicit time stamps.
- **`AnimationController`** – A playback state machine that advances looped clips, exposes playback speed controls,
  and evaluates poses on demand.
- **`AnimationRigPose`** – A structured pose cache with name-based lookup helpers consumed by physics and geometry
  systems.

`make_default_clip()` seeds the runtime with a simple oscillator animation so the end-to-end pipeline can animate
without content pipelines. The `tests/` directory now verifies interpolation correctness.

## Integration notes

- Link against `engine_animation` (CMake target) and include headers from `include/engine/animation/`.
- Controllers are intentionally data-oriented; higher-level blending should layer on top by operating on
  `AnimationRigPose` instances.
- The runtime consumes the exported helpers to keep frame stepping deterministic inside automated tests.

## TODO

- Extend the controller with blend-tree authoring helpers so multiple clips can contribute to the same pose.
- Persist clips to disk (`.anim.json` or similar) to unblock tooling integration.
- Profile interpolation paths and consider SIMD acceleration once more joints are active.
