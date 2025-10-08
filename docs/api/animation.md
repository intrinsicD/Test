# Animation Module

The animation API exposes lightweight structures used to author and evaluate skeletal clips at runtime.
Public declarations live in [`engine/animation/api.hpp`](../../engine/animation/include/engine/animation/api.hpp).

## Core types

- `JointPose` – Stores translation, rotation (quaternion), and scale for an individual joint.
- `Keyframe` / `JointTrack` – Timestamped pose samples for a named joint.
- `AnimationClip` – Aggregates tracks and records the clip duration.
- `AnimationController` – Tracks playback state (time, speed, looping) for a clip.
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

_Last updated: 2025-03-15_
