# Animation Module Roadmap

## Near Term
- Implement additive blend nodes that combine multiple poses before evaluation; extend `AnimationBlendTree` to track additive weights and add regression tests validating pose accumulation.
- Harden clip validation by emitting structured error codes and coverage for edge cases (empty tracks, unordered keys) within `validate_clip` and its unit tests.

## Mid Term
- Integrate GPU/parallel sampling by batching `sample_clip` requests via `compute::KernelDispatcher` and benchmarking controller evaluation throughput.
- Introduce state-machine authoring APIs that orchestrate `AnimationBlendTree` nodes, including transition conditions and event parameter propagation.

## Long Term
- Support advanced deformation pipelines (dual quaternion skinning, curve-driven rigs) by extending pose data structures and generating compatibility layers with geometry/physics representations.
