# Engine Platform Input

## Current State

- Exposes `InputState`, a lightweight tracker for keyboard keys, mouse buttons,
  cursor position, and scroll deltas used by the platform module and tests.
- Enumerates a representative key set sufficient for smoke tests and synthetic
  backends.

## Usage

- Construct `engine::platform::input::InputState` and call `begin_frame()` at the
  start of each frame to snapshot the previous state.
- Feed key and mouse events through the `apply_*` helpers and query the current
  or transient state with `is_key_down`, `was_key_pressed`, and similar accessors.
- Accumulate cursor and scroll information through `apply_cursor_position` and
  `apply_scroll_delta`.

## TODO / Next Steps

- Integrate `InputState` with the windowing backends to surface real device data.
