# Engine Animation Skinning

## Purpose
Implements skinning kernels that map animated rig poses onto mesh vertices for runtime deformation.

## Current State
- **Status:** Scaffolding only
- **Implementation:** CPU reference path targeted for M4 with GPU variants in M5
- Works in concert with deformation utilities to update mesh buffers each frame

## Planned API
```cpp
namespace engine::animation::skinning {
    void update_mesh(const deformation::RigBinding& binding,
                     const AnimationRigPose& pose,
                     geometry::SurfaceMesh& mesh);

    void update_gpu_buffers(const deformation::RigBinding& binding,
                            const AnimationRigPose& pose,
                            rendering::PoseBuffer& buffer);
}
```

## TODO / Next Steps
- [ ] Build CPU linear blend skinning path (@bob, #236, M4)
- [ ] Prototype GPU pose buffer updates (@carol, #237, M4)
- [ ] Add performance benchmarks for large rigs (@animation-team, #237, M5)

## Dependencies
- Requires deformation binding data structures
- Depends on rendering pose buffer abstractions for GPU path
