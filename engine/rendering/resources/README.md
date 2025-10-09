# Engine Rendering Resources

## Current State

- Hosts subdirectories for Buffers, Samplers.
- Provides synchronisation primitives (barriers, timeline semaphores, fences) shared by the frame graph and schedulers.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Implement GPU resource management for buffers, textures, and samplers.
- Extend synchronisation helpers with API-specific metadata (image layouts, access masks).
