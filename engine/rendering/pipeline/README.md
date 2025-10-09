# Engine Rendering Pipeline

## Current State

- Contains scaffolding files that will evolve alongside the subsystem.
- The frame graph schedules passes, records resource lifetime events, and synthesises GPU barriers.
- Execution integrates with the abstract `IGpuScheduler` to issue queue-aware submissions, fences, and timeline semaphores.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Provide concrete backend encoders that translate the scheduler records into API-specific command buffers.
