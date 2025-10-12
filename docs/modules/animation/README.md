# Animation Module

## Current State
- Provides skeletal animation primitives such as `JointPose`, `AnimationClip`, and `AnimationController` along with utilities for keyframe sampling, controller advancement, and blend tree evaluation exposed in `engine/animation/api.hpp`. These routines form the backbone for posing rigs at runtime.
- JSON import/export helpers (`write_clip_json`, `read_clip_json`, `save_clip_json`, `load_clip_json`) enable round-tripping clips for offline tools and automated validation flows.
- Blend-tree authoring helpers cover clip nodes, controller nodes, linear blend nodes, and parameter management (float, bool, event) so higher-level systems can express complex animation graphs.
- Unit tests under `engine/animation/tests/` exercise module loading, serialization, and blend tree behaviours to guard refactors.

## Usage
- Build the static/shared library target with `cmake --build --preset <preset> --target engine_animation` to expose headers under `engine/animation`.
- Link `engine_animation` and its `engine_math` dependency in downstream targets; include `<engine/animation/api.hpp>` to access sampling, validation, and blend-tree helpers.
- Run module tests with `ctest --preset <preset> --tests-regex engine_animation` after configuring with `BUILD_TESTING=ON` to validate behaviour.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
