# Animation Module

## Current State
- Provides skeletal animation primitives such as `JointPose`, `AnimationClip`, and `AnimationController` along with utilities for keyframe sampling, controller advancement, and blend tree evaluation exposed in `engine/animation/api.hpp`. These routines form the backbone for posing rigs at runtime.
- JSON import/export helpers (`write_clip_json`, `read_clip_json`, `save_clip_json`, `load_clip_json`) enable round-tripping clips for offline tools and automated validation flows.
- Blend-tree authoring helpers cover clip nodes, controller nodes, linear and additive blend nodes, and parameter management (float, bool, event) so higher-level systems can express complex animation graphs.
- Unit tests under `engine/animation/tests/` exercise module loading, serialization, and blend tree behaviours to guard refactors.

## Usage
- Build the static/shared library target with `cmake --build --preset <preset> --target engine_animation` to expose headers under `engine/animation`.
- Link `engine_animation` and its `engine_math` dependency in downstream targets; include `<engine/animation/api.hpp>` to access sampling, validation, and blend-tree helpers.
- Run module tests with `ctest --preset <preset> --tests-regex engine_animation` after configuring with `BUILD_TESTING=ON` to validate behaviour.

### Additive blend nodes

`add_additive_blend_node` layers a delta pose on top of a base pose. The additive input is interpreted relative to the identity pose (zero translation, identity rotation, unit scale) and scaled by a weight in $[0, 1]$ before being composed onto the base:

```cpp
using namespace engine::animation;

AnimationBlendTree tree;
const auto base = add_clip_node(tree, idle_clip);
const auto additive = add_clip_node(tree, lean_left_additive_clip);
const auto additive_node = add_additive_blend_node(tree, base, additive, 0.0F);
set_blend_tree_root(tree, additive_node);

const auto weight_param = add_float_parameter(tree, "lean_weight", 0.0F);
bind_additive_blend_weight(tree, additive_node, weight_param);
set_float_parameter(tree, "lean_weight", 0.5F); // Apply 50% of the additive pose
```

Translations accumulate additively, scales are applied as multiplicative offsets around $1.0$, and rotations slerp between identity and the additive pose before being composed with the base.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
