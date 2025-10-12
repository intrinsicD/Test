# Engine Animation Deformation

## Purpose
Contains deformation algorithms that transform mesh vertices based on rig poses.

## Current State
- **Status:** Scaffolding only
- **Implementation:** Planned for M4
- Deformation utilities will consume `AnimationRigPose` and produce transformed mesh data

## Planned API
```cpp
namespace engine::animation::deformation {
    // Apply linear blend skinning
    void apply_lbs(const RigBinding& binding,
                   const AnimationRigPose& pose,
                   geometry::SurfaceMesh& mesh);

    // Apply dual quaternion skinning
    void apply_dqs(const RigBinding& binding,
                   const AnimationRigPose& pose,
                   geometry::SurfaceMesh& mesh);
}
```

## TODO / Next Steps
- [ ] Define RigBinding structure (@bob, #236, M4)
- [ ] Implement LBS algorithm (@bob, #236, M4)
- [ ] Add DQS algorithm (@bob, #237, M5)

## Dependencies
- Requires `engine::geometry::SurfaceMesh`
- Requires matrix palette computation from poses
