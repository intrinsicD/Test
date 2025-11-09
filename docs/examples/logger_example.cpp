#include "engine/core/log.hpp"
#include <spdlog/spdlog.h>

// Example of using the logger in a module
class ExampleModule
{
private:
    std::shared_ptr<spdlog::logger> m_logger;
    int m_frame_count = 0;

public:
    ExampleModule()
    {
        // Create a custom logger for this module
        m_logger = engine::core::Log::create_logger("EXAMPLE");
        m_logger->info("ExampleModule created");
    }

    void initialize()
    {
        m_logger->info("Initializing module...");

        // Simulate some initialization
        m_logger->debug("Setting up resources");
        m_logger->debug("Loading configuration");

        m_logger->info("Module initialization complete");
    }

    void update(float delta_time)
    {
        m_frame_count++;

        // Use trace for very frequent logs
        if (m_frame_count % 60 == 0)
        {
            m_logger->trace("Frame {}, delta: {:.3f}ms", m_frame_count, delta_time * 1000.0f);
        }

        // Warning example
        if (delta_time > 0.033f)
        {
            m_logger->warn("Frame time high: {:.2f}ms (target: 33ms)", delta_time * 1000.0f);
        }
    }

    void shutdown()
    {
        m_logger->info("Shutting down module (processed {} frames)", m_frame_count);
    }
};

int main()
{
    // Initialize the logging system
    engine::core::Log::init();

    // Log from application code
    ENGINE_INFO("Application starting...");

    // Log from engine core
    ENGINE_CORE_INFO("Engine version 1.0.0");
    ENGINE_CORE_DEBUG("Debug mode enabled");

    // Create a module with its own logger
    ExampleModule module;
    module.initialize();

    // Simulate some updates
    for (int i = 0; i < 100; ++i)
    {
        float delta_time = (i % 10 == 0) ? 0.040f : 0.016f;
        module.update(delta_time);
    }

    module.shutdown();

    // Set different log levels
    ENGINE_INFO("Changing log level to 'warn' to reduce verbosity");
    engine::core::Log::set_level(spdlog::level::warn);

    ENGINE_TRACE("This won't appear");
    ENGINE_DEBUG("This won't appear either");
    ENGINE_INFO("This won't appear");
    ENGINE_WARN("This will appear");
    ENGINE_ERROR("This will also appear");

    // Back to info level
    engine::core::Log::set_level(spdlog::level::info);
    ENGINE_INFO("Log level restored to 'info'");

    // Formatted logging examples
    std::string resource_name = "texture.png";
    int resource_size = 1024 * 1024 * 4; // 4 MB
    ENGINE_INFO("Loaded resource '{}' ({:.2f} MB)", resource_name, resource_size / (1024.0f * 1024.0f));

    // Multiple values
    int x = 10, y = 20, z = 30;
    ENGINE_CORE_DEBUG("Position: ({}, {}, {})", x, y, z);

    ENGINE_INFO("Application shutting down...");

    // Shutdown logging
    engine::core::Log::shutdown();

    return 0;
}

