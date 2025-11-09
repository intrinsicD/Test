#pragma once

#include "engine/core/api.hpp"
#include <spdlog/spdlog.h>
#include <memory>
#include <string_view>

namespace engine::core
{
    /**
     * @brief Engine logging system based on spdlog
     *
     * Provides a centralized logging interface for the engine, modules, and plugins.
     * Supports multiple log levels (trace, debug, info, warn, error, critical).
     */
    class ENGINE_CORE_API Log
    {
    public:
        /**
         * @brief Initialize the logging system
         *
         * This should be called once at engine startup before any logging occurs.
         * Creates console and file sinks with appropriate formatting.
         */
        static void init();

        /**
         * @brief Shutdown the logging system
         *
         * Flushes all pending logs and cleans up resources.
         */
        static void shutdown();

        /**
         * @brief Get the core engine logger
         * @return Shared pointer to the core logger
         */
        [[nodiscard]] static std::shared_ptr<spdlog::logger>& get_core_logger();

        /**
         * @brief Get the client/application logger
         * @return Shared pointer to the client logger
         */
        [[nodiscard]] static std::shared_ptr<spdlog::logger>& get_client_logger();

        /**
         * @brief Set the log level for all loggers
         * @param level The log level to set
         */
        static void set_level(spdlog::level::level_enum level);

        /**
         * @brief Create a custom logger with a specific name
         * @param name The name for the logger
         * @return Shared pointer to the new logger
         */
        [[nodiscard]] static std::shared_ptr<spdlog::logger> create_logger(std::string_view name);

    private:
        static std::shared_ptr<spdlog::logger> s_core_logger;
        static std::shared_ptr<spdlog::logger> s_client_logger;
        static bool s_initialized;
    };

} // namespace engine::core

// Core engine logging macros
#define ENGINE_CORE_TRACE(...)    ::engine::core::Log::get_core_logger()->trace(__VA_ARGS__)
#define ENGINE_CORE_DEBUG(...)    ::engine::core::Log::get_core_logger()->debug(__VA_ARGS__)
#define ENGINE_CORE_INFO(...)     ::engine::core::Log::get_core_logger()->info(__VA_ARGS__)
#define ENGINE_CORE_WARN(...)     ::engine::core::Log::get_core_logger()->warn(__VA_ARGS__)
#define ENGINE_CORE_ERROR(...)    ::engine::core::Log::get_core_logger()->error(__VA_ARGS__)
#define ENGINE_CORE_CRITICAL(...) ::engine::core::Log::get_core_logger()->critical(__VA_ARGS__)

// Client/application logging macros
#define ENGINE_TRACE(...)         ::engine::core::Log::get_client_logger()->trace(__VA_ARGS__)
#define ENGINE_DEBUG(...)         ::engine::core::Log::get_client_logger()->debug(__VA_ARGS__)
#define ENGINE_INFO(...)          ::engine::core::Log::get_client_logger()->info(__VA_ARGS__)
#define ENGINE_WARN(...)          ::engine::core::Log::get_client_logger()->warn(__VA_ARGS__)
#define ENGINE_ERROR(...)         ::engine::core::Log::get_client_logger()->error(__VA_ARGS__)
#define ENGINE_CRITICAL(...)      ::engine::core::Log::get_client_logger()->critical(__VA_ARGS__)

