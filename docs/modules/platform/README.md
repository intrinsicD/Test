# Platform Module

## Overview

The platform module provides cross-platform abstractions for windowing, input handling, filesystem access, and hot-reload infrastructure. It supports multiple window backends (GLFW, SDL, Mock) with runtime selection and includes a filesystem watcher for asset hot-reload workflows (`CC-002`).

## Window Backends

### Backend Selection

The platform module supports multiple window backends with automatic fallback:

```cpp
#include "engine/platform/api.hpp"

platform::WindowConfig config{
    .title = "My Application",
    .width = 1920,
    .height = 1080,
    .backend = platform::WindowBackend::Auto,  // Auto-select
    .capabilities = {
        .native_surface = true,
        .headless_compatible = false
    }
};

auto window = platform::create_window(config);
if (!window) {
    fmt::print("Window creation failed\n");
}
```

### Available Backends

| Backend | Headless Safe | Native Surface | Platforms |
| --- | --- | --- | --- |
| **GLFW** | ❌ | ✅ | Windows, Linux, macOS |
| **SDL** | ✅ | ✅ | Windows, Linux, macOS, mobile |
| **Mock** | ✅ | ❌ | All (testing) |

### Runtime Override

Override the default backend via environment variable:

```bash
# Use SDL instead of default
export ENGINE_PLATFORM_WINDOW_BACKEND=sdl

# Force mock backend for headless testing
export ENGINE_PLATFORM_WINDOW_BACKEND=mock
```

Invalid values gracefully degrade to Mock backend to keep automation deterministic.

### Build-Time Configuration

Set the default backend during CMake configuration:

```bash
cmake --preset clang-debug -DENGINE_WINDOW_BACKEND=SDL
```

Options: `GLFW`, `SDL`, `MOCK`, `AUTO` (no build-time preference)

### Backend Implementation

Each backend implements the `IWindowBackend` interface:

```cpp
class IWindowBackend {
public:
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void poll_events() = 0;
    virtual bool should_close() const = 0;
    virtual void swap_buffers() = 0;
    virtual void set_title(std::string_view title) = 0;
    // ... more methods
};
```

## Input Handling

### Keyboard Input

```cpp
auto& input = window->input();

if (input.is_key_pressed(platform::Key::W)) {
    // Move forward
}

if (input.is_key_just_pressed(platform::Key::Space)) {
    // Jump (once per press)
}
```

### Mouse Input

```cpp
auto mouse_pos = input.mouse_position();
auto delta = input.mouse_delta();

if (input.is_mouse_button_pressed(platform::MouseButton::Left)) {
    // Handle click
}

float scroll = input.scroll_delta();
```

### Gamepad Support

```cpp
if (input.is_gamepad_connected(0)) {
    float left_x = input.gamepad_axis(0, platform::GamepadAxis::LeftX);
    
    if (input.is_gamepad_button_pressed(0, platform::GamepadButton::A)) {
        // Handle button press
    }
}
```

## Filesystem Abstraction

### Virtual Filesystem

Mount multiple filesystem providers:

```cpp
#include "engine/platform/filesystem/virtual_filesystem.hpp"

platform::VirtualFilesystem vfs;

// Mount physical directory
vfs.mount("/assets", "C:/MyGame/Assets");

// Mount archive (planned)
vfs.mount("/textures", "textures.pak");

// Read through VFS
auto data = vfs.read_file("/assets/models/character.obj");
```

### Filesystem Watcher

Monitor file changes for hot-reload:

```cpp
#include "engine/platform/filesystem/watcher.hpp"

platform::FilesystemWatcher watcher;

// Watch directory
watcher.watch("assets/", [](const platform::FileEvent& event) {
    if (event.type == platform::FileEventType::Modified) {
        fmt::print("File changed: {}\n", event.path);
        // Trigger asset reload
    }
});

// Poll for events
watcher.poll();
```

Integrates with asset hot-reload (`CC-002`):

```cpp
// Assets module registers callback
watcher.watch("assets/meshes/", [&mesh_cache](const auto& event) {
    if (event.path.extension() == ".obj") {
        mesh_cache.reload(event.path);
    }
});
```

## Time & Clock

High-resolution timing for frame pacing:

```cpp
#include "engine/platform/time.hpp"

auto start = platform::now();
// ... do work ...
auto end = platform::now();

double duration_ms = platform::duration_ms(start, end);
double duration_us = platform::duration_us(start, end);
```

### Frame Limiter

```cpp
platform::FrameLimiter limiter{60.0};  // Target 60 FPS

while (running) {
    limiter.begin_frame();
    
    // Update & render
    
    limiter.end_frame();  // Sleeps to maintain target FPS
    
    double actual_fps = limiter.fps();
}
```

## Threading Support

Platform-specific thread utilities:

```cpp
#include "engine/platform/threading.hpp"

// Get hardware concurrency
size_t thread_count = platform::hardware_concurrency();

// Set thread affinity (platform-specific)
platform::set_thread_affinity(std::this_thread::get_id(), {0, 1});

// Set thread priority
platform::set_thread_priority(platform::ThreadPriority::High);
```

## Clipboard

```cpp
// Set clipboard text
platform::set_clipboard_text("Copied text");

// Get clipboard text
std::string text = platform::get_clipboard_text();
```

## SDL Backend Integration (`PL-215`)

The SDL backend provides parity with GLFW plus additional features:

- Headless rendering support
- Mobile platform support (Android, iOS)
- Better gamepad compatibility
- Audio subsystem integration (planned)

See [`SDL_BACKEND_CHECKLIST.md`](SDL_BACKEND_CHECKLIST.md) for implementation progress.

## Mock Backend (Testing)

The Mock backend enables deterministic testing:

```cpp
platform::WindowConfig config{
    .backend = platform::WindowBackend::Mock
};

auto window = platform::create_window(config);

// Simulate input
auto& mock = static_cast<platform::MockWindow&>(*window);
mock.inject_key_press(platform::Key::W);
mock.inject_mouse_motion(100, 100);
mock.advance_frame();  // Deterministic timing
```

## Platform Detection

Query platform at runtime:

```cpp
#include "engine/platform/platform_info.hpp"

auto platform = platform::current_platform();
switch (platform) {
    case platform::Platform::Windows:
        // Windows-specific code
        break;
    case platform::Platform::Linux:
        // Linux-specific code
        break;
    case platform::Platform::MacOS:
        // macOS-specific code
        break;
}

bool is_desktop = platform::is_desktop_platform();
bool is_mobile = platform::is_mobile_platform();
```

## Error Handling

Platform operations return `Result<T, PlatformError>`:

```cpp
auto result = platform::create_window(config);
if (!result) {
    auto error = result.error();
    fmt::print("Window creation failed: {}\n", error.message);
    
    // Check for missing dependencies
    if (error.code == platform::PlatformError::missing_x11) {
        fmt::print("Install X11 development headers\n");
    }
}
```

## Testing

Platform tests validate:
- Window backend selection and fallback (`test_window.cpp`)
- Input event handling (`test_input.cpp`)
- Filesystem watcher accuracy (`test_watcher.cpp`)
- VFS mounting and resolution (`test_vfs.cpp`)
- Mock backend determinism (`test_mock.cpp`)

Run tests:
```bash
ctest --preset clang-debug -R platform
```

## Dependencies

- **GLFW** (optional): When `ENGINE_ENABLE_GLFW=ON`
- **SDL2** (optional): When SDL backend is compiled
- **X11 libraries** (Linux): `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`
- **Core**: Error handling, telemetry

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones including SDL backend work
- [`SDL_BACKEND_CHECKLIST.md`](SDL_BACKEND_CHECKLIST.md): SDL implementation progress
- [`../../ROADMAP.md`](../../ROADMAP.md): Platform module status in roadmap
- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md): Platform role in data flow
- Used by: Runtime (window management), Assets (filesystem watcher), Rendering (surface creation)

## Current State

- Virtual filesystem providers, filesystem watcher abstraction for hot reload, backend selection plumbing, and mocked window/input services pending OS integrations. Backend runtime override and build-time defaults documented.

## Usage

- Select backend at runtime with `ENGINE_PLATFORM_WINDOW_BACKEND` or at configure-time with `-DENGINE_WINDOW_BACKEND=` as shown above.
- Run platform tests (when enabled):
  - `ctest --preset linux-gcc-debug -R platform`

## TODO / Next Steps

- Scope SDL backend implementation using the parity checklist (`PL-215`) to advance `DC-003`; see ../../ROADMAP.md
- Extend filesystem watcher docs with OS-specific caveats and integration examples in assets hot-reload; see ../../ROADMAP.md
