# Math Module

## Current State
- Provides fundamental vector, matrix, quaternion, and transform types with common operations exposed through `<engine/math/math.hpp>`.
- Includes utilities for random sampling, sparse matrices, and helper functions consumed by geometry, animation, and physics.
- Header-only interface library (`engine_math`) ensures consumers inherit compile definitions without additional linking cost.
- Unit coverage in `engine/math/tests/` validates foundational operations and regressions.

## Usage
- No dedicated build step is required beyond configuring the project; `engine_math` is an interface target pulled automatically by dependants.
- Include `<engine/math/math.hpp>` or specific headers (e.g., `<engine/math/vector.hpp>`) to access functionality.
- Run `ctest --preset <preset> --tests-regex engine_math` with testing enabled to verify core operations when modifying the library.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
