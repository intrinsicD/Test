# Engine Platform Windowing

## Current State

- Contains scaffolding files that will evolve alongside the subsystem.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Flesh out the GLFW and SDL backends so `create_window` produces real native
  windows, pumps their event loops, and surfaces swapchain handles.
- Feed translated keyboard/mouse/device events into `engine::platform::input`
  to remove the current mock-only pipeline.
- Document backend capabilities, configuration flags, and known limitations once
  the integrations stabilise.
