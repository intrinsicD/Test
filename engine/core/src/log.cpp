#include "engine/core/log.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>

namespace engine::core
{
    std::shared_ptr<spdlog::logger> Log::s_core_logger;
    std::shared_ptr<spdlog::logger> Log::s_client_logger;
    bool Log::s_initialized = false;

    void Log::init()
    {
        if (s_initialized)
        {
            return;
        }

        // Create sinks
        std::vector<spdlog::sink_ptr> log_sinks;

        // Console sink with color support
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T] %n: %v%$");
        log_sinks.push_back(console_sink);

        // Rotating file sink (5 MB max size, 3 rotating files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/engine.log", 1024 * 1024 * 5, 3);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %n: %v");
        log_sinks.push_back(file_sink);

        // Create core logger
        s_core_logger = std::make_shared<spdlog::logger>("ENGINE", log_sinks.begin(), log_sinks.end());
        s_core_logger->set_level(spdlog::level::trace);
        s_core_logger->flush_on(spdlog::level::err);
        spdlog::register_logger(s_core_logger);

        // Create client logger
        s_client_logger = std::make_shared<spdlog::logger>("APP", log_sinks.begin(), log_sinks.end());
        s_client_logger->set_level(spdlog::level::trace);
        s_client_logger->flush_on(spdlog::level::err);
        spdlog::register_logger(s_client_logger);

        s_initialized = true;

        ENGINE_CORE_INFO("Logging system initialized");
    }

    void Log::shutdown()
    {
        if (!s_initialized)
        {
            return;
        }

        ENGINE_CORE_INFO("Shutting down logging system");

        spdlog::shutdown();
        s_core_logger.reset();
        s_client_logger.reset();
        s_initialized = false;
    }

    std::shared_ptr<spdlog::logger>& Log::get_core_logger()
    {
        if (!s_initialized)
        {
            init();
        }
        return s_core_logger;
    }

    std::shared_ptr<spdlog::logger>& Log::get_client_logger()
    {
        if (!s_initialized)
        {
            init();
        }
        return s_client_logger;
    }

    void Log::set_level(spdlog::level::level_enum level)
    {
        if (s_core_logger)
        {
            s_core_logger->set_level(level);
        }
        if (s_client_logger)
        {
            s_client_logger->set_level(level);
        }
    }

    std::shared_ptr<spdlog::logger> Log::create_logger(std::string_view name)
    {
        if (!s_initialized)
        {
            init();
        }

        // Use the same sinks as the core logger
        auto logger = std::make_shared<spdlog::logger>(
            std::string(name),
            s_core_logger->sinks().begin(),
            s_core_logger->sinks().end());

        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::err);
        spdlog::register_logger(logger);

        return logger;
    }
} // namespace engine::core
