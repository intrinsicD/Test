# Animation Module

## Current State

- Provides clip, track, and controller primitives with sampling implemented in `src/api.cpp`.
- Supplies a deterministic oscillator clip that powers runtime smoke tests and demonstrations.

## Usage

- Link against `engine_animation` and include `<engine/animation/api.hpp>`; the target inherits the shared `engine::project_options` compile features and contributes its headers to `engine::headers`.
- Extend controllers or clips under `src/` and update accompanying tests in `tests/`.
- Build and test the module with the canonical presets (for example `cmake --preset linux-gcc-debug` followed by `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

- Add blend-tree authoring and clip serialization to unlock complex rigs.
