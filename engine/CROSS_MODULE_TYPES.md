# Cross-Module Type Guidelines

## Problem Statement
Multiple modules (physics, rendering, animation, geometry) consume common math and transform types. Without clear ownership, duplication or cyclic dependencies emerge.

## Recommended Strategy: Shared Foundation Module

We centralise cross-cutting math and transform types inside `engine::math`.

### Types owned by `engine::math`
- `vec2`, `vec3`, `vec4` – Positions, directions, and colours.
- `ivec2`, `ivec3`, `ivec4` – Integer lattice data (indices, grid dimensions).
- `quat` – Orientation representation.
- `mat3`, `mat4` – Linear and affine transforms.
- `Transform` – Position, rotation, scale triple.
- `Color` – RGBA spectral representation for rendering consumers.

### Benefits
- Single source of truth for layout and semantics.
- Eliminates redundant conversion logic between modules.
- Simplifies ABI stability guarantees for public headers.

## Alternative Approaches (When to Consider Them)

1. **Dedicated Interface Module (`engine::core` or similar):**
   - Use when cross-module coordination demands interfaces rather than data (e.g., `ITransformable`).
   - Keeps pure data in `engine::math` while hosting behavioural contracts elsewhere.

2. **Per-Module Types with Adapters:**
   - Acceptable when domains require specialised state (e.g., physics inertia tensors).
   - Provide explicit adapters or conversion functions at integration boundaries.

## Decision Workflow for New Types

1. Is the type pure math without domain rules? → Add to `engine::math`.
2. Is the type required by three or more modules? → Prefer `engine::math` or a new foundational interface module.
3. Is the type domain-specific but shared between two modules? → Place it in the lower-layer module and expose adapters upward.
4. Does the type encapsulate runtime behaviour? → Define an interface in a low-layer service module instead of sharing implementation classes.

## Preventing Type Drift

- Document type semantics (coordinate spaces, handedness, units) alongside definitions.
- Gate API changes through module owners and update this document when ownership shifts.
- Maintain integration tests that exercise serialisation/deserialisation across modules to detect layout drift.

## Versioning and Breaking Changes

- Treat types in `engine::math` as **stable** within an engine major version.
- Introduce new fields via additive changes or versioned wrappers.
- For breaking changes, supply migration helpers and deprecate the old representation over at least one minor release.
