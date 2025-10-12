# Engine Rendering Pipeline

## Current State

- Contains scaffolding files that will evolve alongside the subsystem.
- The frame graph schedules passes, records resource lifetime events, and synthesises GPU barriers.
- Execution integrates with the abstract `IGpuScheduler` to issue queue-aware submissions, fences, and timeline semaphores.
- `RenderExecutionContext` now carries both the asset facing `RenderResourceProvider`, the GPU-facing
  `resources::IGpuResourceProvider`, and a `CommandEncoderProvider` that supplies pass-scoped encoders.

## Data Flow Between Passes and the Scheduler

Render passes receive a `FrameGraphPassExecutionContext` that mirrors the work encoded for the frame. The context exposes:

- `pass_name()` – the logical label propagated through scheduling and submission.
- `reads()` / `writes()` – spans describing the frame-graph resources accessed by the pass.
- `describe(handle)` – immutable metadata (name and lifetime) for any registered resource.
- `command_buffer_handle()` – the logical encoder identifier allocated by the active `IGpuScheduler`.
- `queue_type()` – the queue family selected for execution.
- `command_encoder()` – the pass-local encoder used to record geometry submissions.

Once a pass completes, the frame graph packages its work into a `GpuSubmitInfo` structure that contains the queue
selection, logical command buffer, resource barriers, timeline semaphore waits/signals, and an optional fence. The
`IGpuScheduler` implementation translates this payload into API-specific submissions. Backend adapters obtain the
native queue, command buffer, fence, and semaphore handles from the `resources::IGpuResourceProvider` instance bound
to the frame, while the `CommandEncoderProvider` finalises the recorded commands.

Transient frame-graph resources trigger lifetime notifications via `IGpuResourceProvider::on_transient_acquire` and
`on_transient_release`. Backends can use these callbacks to allocate aliases or stage layout transitions before the
work is submitted.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.
- Refer to `engine/rendering/backend/*/gpu_scheduler.hpp` for examples of translating `GpuSubmitInfo` into native
  submission packets.

## TODO / Next Steps

Align the pipeline work with the rendering roadmap documented in
[`docs/rendering/ROADMAP.md`](../../../../../../docs/rendering/ROADMAP.md) and the
[global alignment overview](../../../../../../docs/global_roadmap.md):

- Extend frame-graph resource descriptions with explicit formats, dimensions,
  and usage flags so backend providers can allocate GPU objects deterministically.
- Propagate queue and command-encoder metadata through
  `FrameGraphPassExecutionContext` to prime the scheduler for backend
  translation.
- Prototype the reference command encoder bridge that replays compiled passes
  into the `IGpuScheduler`, providing the concrete submission hooks tracked in
  the workspace backlog.
