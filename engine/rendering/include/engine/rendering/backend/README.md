# Engine Rendering Backend

## Current State

- Hosts subdirectories for DirectX 12, Metal, OpenGL, Vulkan.
- Each backend now exposes a native `GpuScheduler` adapter that consumes `GpuSubmitInfo` records and translates them
  into API-specific command buffers, queues, fences, and timeline semaphore submissions using the
  `resources::IGpuResourceProvider` contract.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.
- Extend the provided schedulers or replace them with hardware-backed implementations when integrating real devices.

## TODO / Next Steps

- Implement the actual backend integration using the target API.
- Replace the adapters' placeholder handles with real API object creation and submission.
