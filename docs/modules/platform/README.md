# Platform Module

## Overview

The platform module provides cross-platform abstractions for windowing, input handling, filesystem access, and hot-reload infrastructure. It supports multiple window backends (GLFW, Mock) with runtime selection and includes a filesystem watcher for asset hot-reload workflows (`CC-002`).

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
    .capability_requirements = {
        .require_native_surface = true,
        .require_headless_safe = false
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
| **GLFW** | ✅ | ✅ | Windows, Linux, macOS |
| **Mock** | ✅ | ❌ | All (testing) |

When `WindowConfig::CapabilityRequirements::require_headless_safe` is set, the GLFW backend automatically creates a hidden
window so automation can render to off-screen targets without surfacing UI windows. Visibility requests are ignored for these
instances, keeping behaviour consistent across CI and local runs.

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
cmake --preset linux-gcc-debug -DENGINE_WINDOW_BACKEND=GLFW
```

Options: `GLFW`, `MOCK`, `AUTO` (no build-time preference)

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

#### Polling cadence and lifecycle

- `FilesystemWatcher::watch_file()` normalises paths to their absolute,
  lexically normal form before storage, so callbacks always receive a stable
  identifier regardless of the caller's working directory.
- Call `poll()` from a steady cadence. Runtime integrations poll once per frame
  to keep reload latency under 16 ms without producing excessive filesystem
  traffic. Tooling with background worker threads can poll more frequently when
  lower latency is required.
- Only explicitly registered files are monitored. When an asset descriptor moves
  to a new path, unregister the previous watch handle (asset caches perform this
  automatically) so callbacks do not target removed files.

#### Operating-system caveats

- **Windows (NTFS)** – Editors that save via write-to-temp + rename emit a
  `created` event for the replacement followed by an `erased` event for the old
  target. Treat both as reload triggers. Buffered writes that never close the
  handle do not advance the timestamp and therefore do not surface a change;
  ensure tools flush to disk.
- **macOS (APFS)** – Metadata updates are batched. Polling once per frame avoids
  false negatives while keeping CPU usage low. When automated build steps write
  in rapid succession, stagger writes by a few milliseconds so the resulting
  timestamp differs from the previous value.
- **Linux (ext4/XFS)** – Network shares or FAT-formatted removable drives expose
  one-second timestamp precision. Writes that occur within the same second may
  be coalesced; force a timestamp bump with
  `std::filesystem::last_write_time` or insert a short delay between writes.
- **Cross-platform** – Rename sequences surface as `created` + `erased` events.
  Deleted files keep their cached descriptor and most recent timestamp so
  diagnostics tools can surface actionable errors until the source reappears.

#### Asset hot-reload integration

Asset caches bridge watcher events back into cache management. The mesh cache,
for example, binds watcher callbacks that reload the asset and notify listeners:

```cpp
engine::assets::MeshCache meshes;

engine::assets::MeshHandle handle{"meshes/unit_cube"};
engine::assets::MeshAssetDescriptor descriptor{
    .handle = handle,
    .source = "assets/meshes/unit_cube.obj"
};

const auto& mesh = meshes.load(descriptor);

meshes.register_hot_reload_callback(handle, [&](const engine::assets::MeshAsset& reloaded) {
    runtime_scene.rebind_mesh(handle, reloaded.mesh);
});

while (running) {
    meshes.poll();  // Internally invokes FilesystemWatcher::poll()
    runtime_scene.tick();
}
```

`MeshCache::register_watch_locked` wires `FilesystemWatcher::watch_file()` so
modifications trigger `reload_asset()` and invoke registered callbacks, while
erase events simply update the tracked timestamp
([`engine/assets/src/mesh_asset.cpp`](../../../engine/assets/src/mesh_asset.cpp)).

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
ctest --preset linux-gcc-debug -R platform
```

## Dependencies

- **GLFW** (optional): When `ENGINE_ENABLE_GLFW=ON`
-   CMake now disables the option automatically when GLFW cannot be configured
    (missing headers or fetch failures) so builds fall back to the mock backend
    instead of failing during configuration.
- **X11 libraries** (Linux): `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`
- **Core**: Error handling, telemetry

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones
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

- Track upcoming platform backlog (`PL-215` SDL parity, watcher parity on new OS targets) and update [`../../ROADMAP.md`](../../ROADMAP.md) as new tasks are prioritised.
