# Rendering Module

## Current State
- Provides a declarative frame graph API for describing render passes, resources, and dependencies, alongside a forward rendering pipeline implementation.
- Implements command encoder abstractions, resource providers (including a recording GPU resource provider), and a material system coordinating shader bindings.
- Defines backend interfaces and scheduler stubs so platform-specific renderers can plug in while sharing common orchestration.
- Extensive tests in `engine/rendering/tests/` cover frame graph construction, backend adapters, and forward pipeline behaviour.

## Usage
- Build via `cmake --build --preset <preset> --target engine_rendering`; this pulls in core, assets, platform, and scene dependencies.
- Include `<engine/rendering/frame_graph.hpp>`, `<engine/rendering/forward_pipeline.hpp>`, or `<engine/rendering/material_system.hpp>` when constructing render workloads.
- Execute `ctest --preset <preset> --tests-regex engine_rendering` to verify frame graph scheduling and pass correctness after changes.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
