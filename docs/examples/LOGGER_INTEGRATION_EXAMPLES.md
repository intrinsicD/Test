# Real-World Logger Integration Examples

## Example 1: Renderer Module

```cpp
// renderer.hpp
#pragma once
#include "engine/core/log.hpp"
#include <memory>

namespace engine::rendering {

class Renderer {
private:
    std::shared_ptr<spdlog::logger> m_logger;
    
public:
    Renderer() 
        : m_logger(engine::core::Log::create_logger("RENDERER"))
    {
        m_logger->info("Renderer created");
    }
    
    bool initialize() {
        m_logger->info("Initializing renderer...");
        
        if (!create_device()) {
            m_logger->error("Failed to create graphics device");
            return false;
        }
        
        m_logger->info("Graphics device created successfully");
        m_logger->debug("Vendor: {}, Version: {}", get_vendor(), get_version());
        return true;
    }
    
    void render_frame(int frame_number) {
        m_logger->trace("Rendering frame {}", frame_number);
        
        // Rendering logic...
        
        if (get_draw_calls() > 5000) {
            m_logger->warn("High draw call count: {}", get_draw_calls());
        }
    }
};

} // namespace engine::rendering
```

## Example 2: Asset Manager

```cpp
// asset_manager.hpp
#pragma once
#include "engine/core/log.hpp"
#include <string>
#include <chrono>

namespace engine::assets {

class AssetManager {
private:
    std::shared_ptr<spdlog::logger> m_logger;
    
public:
    AssetManager()
        : m_logger(engine::core::Log::create_logger("ASSETS"))
    {
        m_logger->info("Asset Manager initialized");
    }
    
    bool load_texture(const std::string& path) {
        m_logger->debug("Loading texture: {}", path);
        
        if (!file_exists(path)) {
            m_logger->error("Texture file not found: {}", path);
            return false;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Load texture...
        bool success = load_texture_impl(path);
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start);
        
        if (success) {
            m_logger->info("Loaded texture '{}' in {} ms", path, duration.count());
        } else {
            m_logger->error("Failed to load texture '{}' after {} ms", path, duration.count());
        }
        
        return success;
    }
    
    void report_memory_usage() {
        size_t memory_mb = get_memory_usage() / (1024 * 1024);
        
        m_logger->info("Asset memory usage: {} MB", memory_mb);
        
        if (memory_mb > 1024) {
            m_logger->warn("High asset memory usage: {} MB", memory_mb);
        }
    }
};

} // namespace engine::assets
```

## Example 3: Physics System

```cpp
// physics_system.hpp
#pragma once
#include "engine/core/log.hpp"

namespace engine::physics {

class PhysicsSystem {
private:
    std::shared_ptr<spdlog::logger> m_logger;
    int m_body_count = 0;
    
public:
    PhysicsSystem()
        : m_logger(engine::core::Log::create_logger("PHYSICS"))
    {
        m_logger->info("Physics system created");
    }
    
    void initialize(float gravity) {
        m_logger->info("Initializing physics with gravity = {:.2f}", gravity);
        // Initialize...
        m_logger->debug("Physics world created");
    }
    
    void update(float dt) {
        m_logger->trace("Physics update: dt = {:.4f}s, bodies = {}", dt, m_body_count);
        
        // Update physics...
        
        if (m_body_count > 10000) {
            m_logger->warn("High physics body count: {}", m_body_count);
        }
    }
    
    void add_body() {
        m_body_count++;
        m_logger->debug("Added physics body (total: {})", m_body_count);
    }
};

} // namespace engine::physics
```

## Example 4: Main Application

```cpp
// main.cpp
#include "engine/core/log.hpp"
#include "renderer.hpp"
#include "asset_manager.hpp"
#include "physics_system.hpp"
#include <spdlog/spdlog.h>

int main() {
    // Initialize logging
    engine::core::Log::init();
    
    ENGINE_INFO("Application starting...");
    ENGINE_CORE_INFO("Engine version 1.0.0");
    
    // Set appropriate log level for release builds
    #ifdef NDEBUG
        engine::core::Log::set_level(spdlog::level::info);
        ENGINE_INFO("Running in release mode");
    #else
        engine::core::Log::set_level(spdlog::level::debug);
        ENGINE_INFO("Running in debug mode");
    #endif
    
    // Create subsystems
    engine::rendering::Renderer renderer;
    engine::assets::AssetManager assets;
    engine::physics::PhysicsSystem physics;
    
    // Initialize
    if (!renderer.initialize()) {
        ENGINE_CRITICAL("Failed to initialize renderer");
        return 1;
    }
    
    assets.load_texture("assets/textures/player.png");
    physics.initialize(9.81f);
    
    // Main loop
    ENGINE_INFO("Entering main loop");
    for (int frame = 0; frame < 1000; ++frame) {
        physics.update(0.016f);
        renderer.render_frame(frame);
        
        if (frame % 100 == 0) {
            assets.report_memory_usage();
        }
    }
    
    ENGINE_INFO("Application shutting down");
    engine::core::Log::shutdown();
    
    return 0;
}
```

## Example 5: Plugin System

```cpp
// plugin_interface.hpp
#pragma once
#include "engine/core/log.hpp"

namespace engine::plugins {

class IPlugin {
protected:
    std::shared_ptr<spdlog::logger> m_logger;
    
public:
    virtual ~IPlugin() = default;
    virtual void on_load() = 0;
    virtual void on_unload() = 0;
};

// Example plugin implementation
class AudioPlugin : public IPlugin {
public:
    AudioPlugin() {
        m_logger = engine::core::Log::create_logger("AUDIO_PLUGIN");
        m_logger->info("Audio plugin constructed");
    }
    
    void on_load() override {
        m_logger->info("Loading audio plugin...");
        
        if (!initialize_audio_device()) {
            m_logger->error("Failed to initialize audio device");
            return;
        }
        
        m_logger->info("Audio plugin loaded successfully");
        m_logger->debug("Sample rate: 44100 Hz, Channels: 2");
    }
    
    void on_unload() override {
        m_logger->info("Unloading audio plugin");
        shutdown_audio_device();
    }
};

} // namespace engine::plugins
```

## Example 6: Error Handling with Logging

```cpp
#include "engine/core/log.hpp"
#include <stdexcept>

class ResourceLoader {
private:
    std::shared_ptr<spdlog::logger> m_logger;
    
public:
    ResourceLoader()
        : m_logger(engine::core::Log::create_logger("RESOURCE"))
    {}
    
    void load_resources() {
        try {
            m_logger->info("Loading resources...");
            
            load_shaders();
            load_textures();
            load_models();
            
            m_logger->info("All resources loaded successfully");
        }
        catch (const std::runtime_error& e) {
            m_logger->error("Runtime error while loading resources: {}", e.what());
            throw;
        }
        catch (const std::exception& e) {
            m_logger->critical("Critical exception: {}", e.what());
            throw;
        }
    }
    
private:
    void load_shaders() {
        m_logger->debug("Loading shaders...");
        // Implementation...
    }
    
    void load_textures() {
        m_logger->debug("Loading textures...");
        // Implementation...
    }
    
    void load_models() {
        m_logger->debug("Loading models...");
        // Implementation...
    }
};
```

## Tips for Module Integration

1. **Create logger in constructor**
   ```cpp
   MyClass() : m_logger(engine::core::Log::create_logger("MYCLASS")) {}
   ```

2. **Use appropriate log levels**
   - TRACE: Per-frame or per-iteration details
   - DEBUG: Initialization, configuration, state changes
   - INFO: Major milestones, successful operations
   - WARN: Recoverable issues, performance warnings
   - ERROR: Failures that don't crash the app
   - CRITICAL: Severe errors requiring immediate attention

3. **Format messages clearly**
   ```cpp
   m_logger->info("Loaded {} textures in {:.2f}ms", count, time_ms);
   ```

4. **Include context in errors**
   ```cpp
   m_logger->error("Failed to load '{}': {}", filename, error_message);
   ```

5. **Performance considerations**
   ```cpp
   // Use trace for high-frequency logs
   m_logger->trace("Processing entity {}", entity_id);
   
   // Can be disabled in release builds
   #ifdef NDEBUG
       engine::core::Log::set_level(spdlog::level::info);
   #endif
   ```

