# Engine Scene Serialization

## Current State

- Exposes the serializer API used by `engine::scene::Scene::save/load`.
- Supports round-tripping the built-in components (Name, Hierarchy, Local/World/Dirty transform) with deterministic entity identifiers.
- Provides helper utilities that rebuild parent/child relationships after deserialization.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Extend the encoding to support upcoming runtime components (lights, cameras, visibility state) without breaking existing data.
- Introduce explicit format versioning and compatibility shims for forward/backward migration.
- Document the text representation and add binary/streaming variants once the format stabilises.
