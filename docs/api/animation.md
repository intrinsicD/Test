# Animation Module

The animation API exposes lightweight structures used to author and evaluate skeletal clips at runtime.
Public declarations live in [`engine/animation/api.hpp`](../../engine/animation/include/engine/animation/api.hpp).

## Core types

- `JointPose` – Stores translation, rotation (quaternion), and scale for an individual joint.
- `Keyframe` / `JointTrack` – Timestamped pose samples for a named joint.
- `AnimationClip` – Aggregates tracks and records the clip duration.
- `AnimationController` – Tracks playback state (time, speed, looping) for a clip.
- `AnimationBlendTree` – Node-based graph that evaluates clips and blend operators.
- `BlendTreeParameter` – Runtime parameter binding (float, bool, event) surfaced to the host application.
- `AnimationRigPose` – Container of joint poses with constant-time name lookup via `find()`.

## Helper routines

```cpp
#include <engine/animation/api.hpp>

auto clip = engine::animation::make_default_clip();
auto controller = engine::animation::make_linear_controller(std::move(clip));
engine::animation::advance_controller(controller, 0.016);
auto pose = engine::animation::evaluate_controller(controller);
```

`make_default_clip()` returns a procedural oscillation clip used by the runtime smoke test. `make_linear_controller`
validates keyframe ordering and normalises the clip duration before returning a controller ready for playback.

`AnimationRigPose::find("root")` returns the active joint pose; this is consumed directly by the physics and geometry
subsystems to influence forces and mesh deformation.

## Blend Tree API

Blend trees expose composable nodes that mix clips and controller output based on parameter binding.

```cpp
using namespace engine::animation;

AnimationBlendTree tree;
auto idle = add_clip_node(tree, load_clip("idle.json"));
auto walk = add_clip_node(tree, load_clip("walk.json"));
auto blend = add_linear_blend_node(tree, idle, walk, 0.5f);
set_blend_tree_root(tree, blend);

// Bind parameter to control blend weight
auto speed_param = add_float_parameter(tree, "speed", 0.0f);
bind_linear_blend_weight(tree, blend, speed_param);

// Evaluate
advance_blend_tree(tree, dt);
auto pose = evaluate_blend_tree(tree);
```

`add_clip_node` stores the clip handle in the tree and returns a node identifier. `add_linear_blend_node`
creates a blend operator that mixes two input nodes according to a bound parameter or constant weight.
Parameters act as the runtime control surface and are advanced alongside the blend tree so callers can
drive weights, events, and boolean gates in lockstep with the host application.

Last updated: 2025-10-12
