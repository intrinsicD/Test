# Engine Rendering Resources

## Current State

- Hosts subdirectories for Buffers and Samplers.
- Implementation files live here, while the shared synchronisation primitives are exported from
  `include/engine/rendering/resources`.
- Defines the backend-neutral `resources::IGpuResourceProvider` interface together with a
  `RecordingGpuResourceProvider` implementation used by tests to surface API-native handles and lifetime hooks.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.
- Consumers that implement a backend should derive from `IGpuResourceProvider` to translate frame-graph events into
  native allocations and to service scheduler requests for queues, command buffers, fences, and timeline semaphores.

## TODO / Next Steps

- Implement GPU resource management for buffers, textures, and samplers.
- Extend synchronisation helpers with API-specific metadata (image layouts, access masks) in the exported headers.
- Provide concrete providers for Vulkan, DirectX 12, Metal, and OpenGL that allocate real GPU objects.
