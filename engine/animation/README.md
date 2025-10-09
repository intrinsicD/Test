# Animation Module

## Current State

- Provides clip, track, and controller primitives with sampling implemented in `src/api.cpp`.
- Supplies a deterministic oscillator clip that powers runtime smoke tests and demonstrations.

## Usage

- Link against `engine_animation` and include `<engine/animation/api.hpp>` to access the API.
- Extend controllers or clips under `src/` and update accompanying tests in `tests/`.

## TODO / Next Steps

- Add blend-tree authoring and clip serialization to unlock complex rigs.
