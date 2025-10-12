# Platform Module Roadmap

## Near Term
- Replace the mock-first backend selection with real GLFW/SDL implementations by default, adding runtime detection and configuration flags to fall back when unavailable.
- Harden filesystem helpers with write/watch capabilities to support asset hot reload and document platform-specific behaviour.

## Mid Term
- Expose high-DPI, fullscreen, and swapchain surface negotiation options for renderer integration; expand tests to cover backend-specific quirks.
- Integrate comprehensive input sampling (keyboard, mouse, gamepad) with event buffering and deadzone handling.

## Long Term
- Deliver telemetry hooks (event tracing, frame timing) and remote control channels to support editor tooling and automated testing harnesses.
