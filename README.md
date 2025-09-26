# 3G Rendering and Geometry Processing Engine

This repository hosts the scaffold for a highly modular rendering and geometry processing engine. The layout is organized to separate platform, runtime, rendering, geometry, and tooling concerns so that each subsystem can evolve independently.

## Directory Structure
```
.
├── docs/
├── engine/
│   ├── animation/
│   │   ├── include/
│   │   ├── src/
│   │   ├── deformation/
│   │   ├── rigging/
│   │   └── skinning/
│   ├── assets/
│   │   ├── include/
│   │   ├── src/
│   │   ├── samples/
│   │   └── shaders/
│   ├── compute/
│   │   ├── include/
│   │   ├── src/
│   │   └── cuda/
│   │       ├── include/
│   │       └── src/
│   ├── core/
│   │   ├── include/
│   │   ├── src/
│   │   ├── application/
│   │   ├── configuration/
│   │   ├── diagnostics/
│   │   ├── ecs/
│   │   ├── memory/
│   │   ├── parallel/
│   │   ├── plugin/
│   │   └── runtime/
│   ├── geometry/
│   │   ├── include/
│   │   ├── src/
│   │   ├── csg/
│   │   ├── decimation/
│   │   ├── mesh/
│   │   ├── surfaces/
│   │   ├── topology/
│   │   ├── uv/
│   │   └── volumetric/
│   ├── io/
│   │   ├── include/
│   │   ├── src/
│   │   ├── cache/
│   │   ├── exporters/
│   │   └── importers/
│   ├── physics/
│   │   ├── include/
│   │   ├── src/
│   │   ├── collision/
│   │   └── dynamics/
│   ├── platform/
│   │   ├── include/
│   │   ├── src/
│   │   ├── filesystem/
│   │   ├── input/
│   │   └── windowing/
│   ├── rendering/
│   │   ├── include/
│   │   ├── src/
│   │   ├── backend/
│   │   │   ├── directx12/
│   │   │   ├── metal/
│   │   │   ├── opengl/
│   │   │   └── vulkan/
│   │   ├── lighting/
│   │   ├── materials/
│   │   │   ├── shaders/
│   │   │   └── textures/
│   │   ├── passes/
│   │   ├── pipeline/
│   │   ├── resources/
│   │   │   ├── buffers/
│   │   │   └── samplers/
│   │   └── visibility/
│   ├── scene/
│   │   ├── include/
│   │   ├── src/
│   │   ├── components/
│   │   ├── graph/
│   │   ├── serialization/
│   │   └── systems/
│   ├── tests/
│   │   ├── integration/
│   │   ├── performance/
│   │   └── unit/
│   └── tools/
│       ├── editor/
│       ├── pipelines/
│       └── profiling/
└── scripts/
```

Each subsystem now contains `include/` and `src/` directories with a minimal API surface so the project can be built module-by-module via CMake while deeper feature folders remain placeholders for future implementation work.

## Building

```bash
cmake -S . -B build
cmake --build build
```

The root CMake configuration produces a static library for every engine subsystem (animation, assets, compute, core, geometry, IO, physics, platform, rendering, and scene). The compute subsystem wraps the CUDA module so GPU-accelerated code can be consumed by rendering, geometry processing, or any other part of the engine without being constrained to the rendering backend hierarchy.
