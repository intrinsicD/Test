# Engine Logging System

The engine provides a comprehensive logging system based on spdlog that can be used throughout the engine, in modules, and in plugins.

## Features

- **Multiple log levels**: trace, debug, info, warn, error, critical
- **Dual loggers**: Separate loggers for engine core and client/application code
- **Formatted output**: Support for fmt-style formatting
- **Multiple sinks**: Console output (with colors) and rotating file logs
- **Thread-safe**: Safe to use from multiple threads
- **Custom loggers**: Create named loggers for specific subsystems

## Quick Start

### Basic Usage

```cpp
#include "engine/core/log.hpp"

int main()
{
    // Initialize the logging system (optional - auto-initializes on first use)
    engine::core::Log::init();
    
    // Use core logger for engine internals
    ENGINE_CORE_INFO("Engine starting up...");
    ENGINE_CORE_WARN("This is a warning from the engine");
    ENGINE_CORE_ERROR("This is an error from the engine");
    
    // Use client logger for application code
    ENGINE_INFO("Application initialized");
    ENGINE_WARN("Low memory warning");
    ENGINE_ERROR("Failed to load asset");
    
    // Shutdown logging system
    engine::core::Log::shutdown();
    
    return 0;
}
```

### Formatted Logging

The logger supports fmt-style formatting:

```cpp
int frame_count = 1000;
float fps = 60.5f;
std::string scene_name = "MainScene";

ENGINE_INFO("Frame {}: FPS = {:.2f}", frame_count, fps);
ENGINE_CORE_DEBUG("Loading scene: {}", scene_name);
ENGINE_CORE_INFO("Position: ({}, {}, {})", x, y, z);
```

### Log Levels

```cpp
// Trace - Very detailed information, typically only for debugging
ENGINE_TRACE("Entering function foo()");
ENGINE_CORE_TRACE("Processing vertex at index {}", i);

// Debug - Detailed information for debugging
ENGINE_DEBUG("Cache hit for resource: {}", resource_id);
ENGINE_CORE_DEBUG("Shader compiled in {} ms", compile_time);

// Info - General informational messages
ENGINE_INFO("Application started successfully");
ENGINE_CORE_INFO("Renderer initialized: OpenGL 4.6");

// Warn - Warning messages for potentially harmful situations
ENGINE_WARN("Using deprecated API function");
ENGINE_CORE_WARN("Texture size exceeds recommended maximum");

// Error - Error messages for failures that don't require immediate shutdown
ENGINE_ERROR("Failed to load texture: {}", filename);
ENGINE_CORE_ERROR("GPU memory allocation failed");

// Critical - Critical errors that may require shutdown
ENGINE_CRITICAL("Fatal error: Cannot initialize graphics device");
ENGINE_CORE_CRITICAL("Out of memory");
```

### Setting Log Levels

You can adjust the minimum log level at runtime:

```cpp
#include <spdlog/spdlog.h>

// Set to info level (hide trace and debug)
engine::core::Log::set_level(spdlog::level::info);

// Set to debug level
engine::core::Log::set_level(spdlog::level::debug);

// Set to trace level (show everything)
engine::core::Log::set_level(spdlog::level::trace);

// Set to error level (only errors and critical)
engine::core::Log::set_level(spdlog::level::err);
```

### Custom Loggers for Modules/Plugins

Create named loggers for specific subsystems:

```cpp
// In your module initialization
class PhysicsModule
{
private:
    std::shared_ptr<spdlog::logger> m_logger;

public:
    PhysicsModule()
    {
        m_logger = engine::core::Log::create_logger("PHYSICS");
        m_logger->info("Physics module initialized");
    }
    
    void update()
    {
        m_logger->trace("Updating physics simulation");
        m_logger->debug("Processing {} rigid bodies", body_count);
    }
};

// In a plugin
class MyPlugin
{
private:
    std::shared_ptr<spdlog::logger> m_logger;

public:
    MyPlugin()
    {
        m_logger = engine::core::Log::create_logger("MYPLUGIN");
        m_logger->info("Plugin loaded");
    }
};
```

### Direct Logger Access

You can also access the loggers directly for advanced usage:

```cpp
auto& core_logger = engine::core::Log::get_core_logger();
auto& client_logger = engine::core::Log::get_client_logger();

// Use spdlog API directly
core_logger->log(spdlog::level::info, "Direct log message");
client_logger->flush(); // Flush immediately
```

## Log Output

### Console Output
Logs appear in the console with colors:
```
[12:34:56] ENGINE: Engine starting up...
[12:34:56] APP: Application initialized
```

### File Output
Logs are also written to `logs/engine.log` with detailed timestamps:
```
[2025-11-09 12:34:56.123] [info] ENGINE: Engine starting up...
[2025-11-09 12:34:56.124] [info] APP: Application initialized
```

The file logger uses rotating files (5 MB max, 3 backups) to prevent unlimited disk usage.

## Best Practices

1. **Use appropriate log levels**: Don't use INFO for debug information or ERROR for warnings
2. **Be descriptive**: Include context in your log messages
3. **Use formatting**: Format values into messages for clarity
4. **Module-specific loggers**: Create custom loggers for major subsystems
5. **Don't log in tight loops**: Avoid excessive logging in performance-critical code
6. **Include units**: When logging measurements, include units (ms, MB, etc.)

## Examples in Different Contexts

### In a Renderer

```cpp
#include "engine/core/log.hpp"

class Renderer
{
private:
    std::shared_ptr<spdlog::logger> m_logger;

public:
    Renderer()
    {
        m_logger = engine::core::Log::create_logger("RENDERER");
        m_logger->info("Renderer created");
    }
    
    void initialize()
    {
        m_logger->info("Initializing renderer...");
        
        if (!create_device())
        {
            m_logger->error("Failed to create graphics device");
            return;
        }
        
        m_logger->info("Graphics device created successfully");
        m_logger->debug("Vendor: {}, Version: {}", vendor, version);
    }
    
    void render_frame()
    {
        m_logger->trace("Rendering frame {}", frame_number);
        
        if (draw_calls > 5000)
        {
            m_logger->warn("High draw call count: {}", draw_calls);
        }
    }
};
```

### In an Asset Loader

```cpp
class AssetLoader
{
private:
    std::shared_ptr<spdlog::logger> m_logger;

public:
    AssetLoader()
    {
        m_logger = engine::core::Log::create_logger("ASSETS");
    }
    
    bool load_texture(const std::string& path)
    {
        m_logger->debug("Loading texture: {}", path);
        
        if (!file_exists(path))
        {
            m_logger->error("Texture file not found: {}", path);
            return false;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Load texture...
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start);
        
        m_logger->info("Loaded texture {} in {} ms", path, duration.count());
        return true;
    }
};
```

## Integration with Existing Code

To add logging to existing engine code, simply include the header and use the macros:

```cpp
#include "engine/core/log.hpp"

// In engine code
ENGINE_CORE_INFO("Initializing subsystem");

// In application code
ENGINE_INFO("Loading level");
```

No additional setup is required - the logging system auto-initializes on first use.

