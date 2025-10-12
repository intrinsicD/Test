# Engine Animation Rigging

## Purpose
Hosts rig definition utilities that describe joint hierarchies, inverse bind poses, and metadata consumed during deformation.

## Current State
- **Status:** Scaffolding only
- **Implementation:** Binding authoring structures targeted for M3–M4
- Rig data structures mirror `AnimationRigPose` layouts to minimise conversion cost

## Planned API
```cpp
namespace engine::animation::rigging {
    struct RigDefinition {
        std::vector<std::string> joint_names;
        std::vector<math::mat4> inverse_bind_poses;
        std::vector<int> parent_indices;
    };

    RigDefinition load_rig(const io::path& source);
    RigDefinition make_rig_from_pose(const AnimationRigPose& pose);
}
```

## TODO / Next Steps
- [ ] Finalise rig serialization schema (@bob, #236, M4)
- [ ] Implement rig validation against clips (@alice, #235, M3)
- [ ] Author sample rigs for regression tests (@animation-team, #238, M4)

## Dependencies
- Relies on IO module for rig serialization
- Shares math utilities for matrix/quaternion conversions
