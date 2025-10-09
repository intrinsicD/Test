# Engine Rendering Resources

## Current State

- Hosts subdirectories for Buffers and Samplers.
- Implementation files live here, while the shared synchronisation primitives are exported from
  `include/engine/rendering/resources`.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Implement GPU resource management for buffers, textures, and samplers.
- Extend synchronisation helpers with API-specific metadata (image layouts, access masks) in the exported headers.
