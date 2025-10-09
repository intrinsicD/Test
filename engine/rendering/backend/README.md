# Engine Rendering Backend

## Current State

- Hosts subdirectories for DirectX 12, Metal, OpenGL, Vulkan.
- Each backend exposes a stub `GpuScheduler` that records submissions and models queue selection semantics.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Implement the actual backend integration using the target API.
- Replace the stub schedulers with hardware-backed command queue implementations.
