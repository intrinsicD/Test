# Animation Module

## Overview

The animation module provides deterministic clip sampling, blend-tree evaluation, animation controllers, and skeletal deformation support. It generates pose data consumed by the geometry module for linear blend skinning and feeds into the runtime orchestration layer.

## Core Components

### Animation Clips

`AnimationClip` stores keyframe data organized by joint tracks:

```cpp
#include "engine/animation/api.hpp"

animation::AnimationClip clip{
    .name = "walk_cycle",
    .duration = 1.0,
    .tracks = {
        animation::JointTrack{
            .joint_name = "hip",
            .keyframes = {
                {.time = 0.0, .pose = {.translation = {0, 1, 0}}},
                {.time = 0.5, .pose = {.translation = {0, 1.2, 0}}},
                {.time = 1.0, .pose = {.translation = {0, 1, 0}}}
            }
        }
    }
};
```

### Animation Controllers

Controllers manage playback state and time progression:

```cpp
animation::AnimationController controller{
    .clip = clip,
    .playback_time = 0.0,
    .playback_speed = 1.0,
    .looping = true
};

// Advance time
controller.playback_time += delta_time * controller.playback_speed;
if (controller.looping && controller.playback_time > clip.duration) {
    controller.playback_time = std::fmod(controller.playback_time, clip.duration);
}
```

The module provides factory functions for common patterns:
- `make_default_clip()`: Creates a simple test animation
- `make_linear_controller(clip)`: Returns a controller with standard looping behavior

### Pose Sampling

Sample clips at specific times to generate joint poses:

```cpp
animation::AnimationRigPose pose = animation::sample_clip(clip, playback_time);

// Query individual joints
if (const auto* joint_pose = pose.find("hip")) {
    math::vec3 translation = joint_pose->translation;
    math::quat rotation = joint_pose->rotation;
    math::vec3 scale = joint_pose->scale;
}
```

Poses are deterministic for identical inputs, making them suitable for networked simulations and regression testing.

### Blend Trees

Combine multiple animations using hierarchical blend trees:

```cpp
animation::AnimationBlendTree tree;
tree.nodes = {
    // Node 0: Walk clip
    animation::BlendTreeNode{animation::BlendTreeClipNode{walk_controller}},
    // Node 1: Run clip
    animation::BlendTreeNode{animation::BlendTreeClipNode{run_controller}},
    // Node 2: Linear blend between walk and run
    animation::BlendTreeNode{animation::BlendTreeLinearBlendNode{
        .lhs = 0,
        .rhs = 1,
        .weight = 0.5f  // 50% walk, 50% run
    }}
};

tree.root_node = 2;
tree.parameters = {
    animation::BlendTreeParameter{
        .name = "speed",
        .type = animation::BlendTreeParameterType::kFloat,
        .float_value = 0.5f
    }
};

auto blended_pose = animation::evaluate_blend_tree(tree, delta_time);
```

Blend tree nodes support:
- **ClipNode**: Direct animation playback
- **LinearBlendNode**: Weighted blend between two nodes (lerp/slerp)
- **AdditiveNode**: Layered additive blending for corrections
- **Parameter-driven weights**: Link blend weights to runtime parameters

### Rig Binding & Deformation

Connect animation poses to geometry through rig bindings:

```cpp
animation::RigBinding binding = animation::create_rig_binding(
    skeleton_joint_names,
    mesh
);

// Validate binding
if (animation::validate_binding(binding, mesh) != animation::BindingValidationResult::Valid) {
    // Handle binding errors
}

// Generate deformation transforms
auto transforms = animation::compute_skinning_transforms(pose, binding);

// Apply to mesh (typically done by geometry module)
geometry::apply_linear_blend_skinning(mesh, transforms, binding.joint_weights);
```

## Serialization

Import and export clips to JSON format:

```cpp
// Export
std::ofstream out("animation.json");
animation::write_clip_json(clip, out);

// Import
std::ifstream in("animation.json");
auto result = animation::read_clip_json(in);
if (result) {
    animation::AnimationClip loaded_clip = std::move(*result);
}
```

Serialization uses the error handling pattern from `DC-004`, returning `Result<T, Error>` types for structured error reporting.

## Validation

The module provides validation for clips, controllers, and bindings:

```cpp
auto validation_result = animation::validate_clip(clip);
if (!validation_result.is_valid) {
    for (const auto& error : validation_result.errors) {
        fmt::print("Validation error: {}\n", error.message);
    }
}
```

Validation checks:
- Keyframe temporal ordering within tracks
- Duration consistency across the clip
- Joint name uniqueness
- Rig binding compatibility with mesh topology
- Transform decomposition validity (scale/rotation/translation)

## Integration with Runtime

The runtime module consumes animation poses during its tick cycle:

1. **Controller evaluation**: Runtime advances playback time based on delta time
2. **Pose generation**: Sample clips or evaluate blend trees
3. **Deformation**: Generate skinning matrices from pose data
4. **Geometry update**: Apply transforms to mesh vertices (via geometry module)
5. **Telemetry**: Report evaluation timing in runtime diagnostics

See [`../runtime/README.md`](../runtime/README.md) for orchestration details.

## Performance Considerations

- **Deterministic sampling**: Clip evaluation is deterministic and suitable for parallel execution
- **Cache-friendly layout**: Keyframes are stored sequentially per joint for optimal prefetching
- **SIMD opportunities**: Transform math uses the math module's vectorized types
- **GPU sampling (active)**: `AN-230` is executing GPU-parallel sampling benchmarks via compute shaders using dispatcher telemetry
- **Benchmark harness**: `engine_animation_benchmark_driver` captures CPU baselines and emits dispatcher-compatible telemetry. Build it with `cmake --build --preset <preset> --target engine_animation_benchmark_driver` and run with `--output` to produce JSON payloads for `compute_dispatch_report.py`.

Current benchmarks (from `T-0113`):
- CPU LBS: ~0.8ms per frame for 1000-vertex mesh with 20 joints
- Pose sampling: ~0.05ms per clip with 10 joints

## Testing

Tests cover:
- Clip validation and serialization (`test_clip_serialization.cpp`)
- Controller evaluation and looping (`test_module.cpp`)
- Blend tree evaluation (`test_blend_tree.cpp`)
- Rig binding validation (`test_rig_binding.cpp`)
- Linear blend skinning accuracy (`test_deformation.cpp`)

Run animation tests:
```bash
ctest --preset linux-gcc-debug -R animation
```

## Dependencies

- **Math**: Vector, quaternion, matrix types and transform utilities
- **Geometry** (integration): Mesh data structures for deformation
- **IO** (optional): File format handlers for animation import/export
- **Runtime** (integration): Lifecycle orchestration and telemetry

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module-specific milestones and upcoming features
- [`../../specs/ADR-0006-animation-deformation.md`](../../specs/ADR-0006-animation-deformation.md): Deformation architecture decisions
- [`../../specs/AN-240-state-machine-authoring.md`](../../specs/AN-240-state-machine-authoring.md): Planned state machine design
- [`../../design/ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md`](../../design/ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md): GPU sampling proposal
- [`../../tasks/T-0113-animation-runtime-skinning.md`](../../tasks/T-0113-animation-runtime-skinning.md): Runtime integration milestone

## Current State

- Deterministic clip sampling, controllers, blend trees, rig binding, and CPU linear blend skinning are implemented with tests.
- JSON import/export and validation flows are available; error handling follows `DC-004` using `Result<T, Error>`.

## Usage

- Build and run animation tests:
  - `ctest --preset linux-gcc-debug -R animation`
- Sample code for clip sampling, blend trees, and deformation is included above; see C++ tests under `engine/animation/tests/` for end-to-end patterns.

## TODO / Next Steps

- Track GPU/parallel sampling benchmarks (`AN-230`) as the current roadmap focus; see ../../ROADMAP.md
- Coordinate with runtime for sampling telemetry and potential compute offload interfaces.
