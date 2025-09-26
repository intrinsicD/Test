

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
│   ├── runtime/
│   │   ├── include/
│   │   └── src/
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

The root CMake configuration produces a shared library for every engine subsystem (animation, assets, compute, core, geometry, IO, physics, platform, rendering, scene, and the aggregate runtime). The compute subsystem wraps the CUDA module so GPU-accelerated code can be consumed by rendering, geometry processing, or any other part of the engine without being constrained to the rendering backend hierarchy.

### Python Interoperability

Building shared libraries enables lightweight bindings that can be consumed from Python via the helper package shipped in `python/engine3g`. After compiling the project, point the helper at the build output directory and load modules dynamically:

```python
>>> from engine3g import load_runtime
>>> runtime = load_runtime()
Traceback (most recent call last):
    ...
RuntimeError: Unable to locate the shared library 'libengine_runtime.so'. Set ENGINE3G_LIBRARY_PATH or provide explicit search paths.
>>> import os, pathlib
>>> os.environ["ENGINE3G_LIBRARY_PATH"] = str(pathlib.Path("build").resolve())
>>> runtime = load_runtime()
>>> runtime.name()
'runtime'
>>> runtime.module_names()
['animation', 'assets', 'compute', 'compute.cuda', 'core', 'geometry', 'io', 'physics', 'platform', 'rendering', 'scene']
>>> modules = runtime.load_modules()
>>> modules['core'].resolved_name()
'core'
```

The loader searches directories listed in `ENGINE3G_LIBRARY_PATH`, the current working directory, and the `engine3g` package folder itself. You can also pass explicit paths to `load_runtime`, `load_module`, or `load_all_modules` if you prefer not to use environment variables.

### Coding Style

Refer to [`CODING_STYLE.md`](./CODING_STYLE.md) for detailed guidance when authoring C++ modules or Python utilities. Adhering to these conventions keeps the engine consistent across languages and simplifies code review.
