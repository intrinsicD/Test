# Animation Module

## Current State

- Provides clip, track, and controller primitives with sampling implemented in `src/api.cpp`.
- Supplies a deterministic oscillator clip that powers runtime smoke tests and demonstrations.
- Validates clips for structural issues and serialises/deserialises them as JSON to integrate with external asset tooling.
- Public headers expose the core data structures consumed by other subsystems; implementation is kept ABI-stable for external tooling.

## Usage

- Link against `engine_animation` and include `<engine/animation/api.hpp>`; the target inherits the shared `engine::project_options` compile features and contributes its headers to `engine::headers`.
- Use `write_clip_json`/`read_clip_json` (or the filesystem helpers) to persist clips and `validate_clip` before exporting author-authored data.
- Extend controllers or clips under `src/` and update accompanying tests in `tests/`.
- Build and test the module with the canonical presets (for example `cmake --preset linux-gcc-debug` followed by `ctest --preset linux-gcc-debug`).

## Roadmap

The high-level roadmap is captured in [docs/animation_roadmap.md](../../docs/animation_roadmap.md) and summarised in the
[global alignment overview](../../docs/global_roadmap.md). The immediate sequencing is:

1. **Harden clip assets** – Validation and JSON serialization are now available; next steps wire the APIs into the I/O module for broader asset flows.
2. **Author blend trees** – Introduce a node graph with evaluators that blend multiple clips and procedural nodes at runtime.
3. **Integrate deformation** – Provide rig binding metadata and deformation utilities that interface with geometry and rendering.
4. **Expand coverage** – Build profiling scenarios and broaden the test suite to exercise non-looping playback, edge cases, and performance hot spots.

## TODO / Next Steps

- Execute the roadmap steps above to enable complex rig playback, blending, and deformation workflows.
