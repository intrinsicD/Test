#include "engine/core/log.hpp"
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

namespace engine::core::tests
{
    class LogTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            Log::init();
        }

        void TearDown() override
        {
            Log::shutdown();
        }
    };

    TEST_F(LogTest, InitializationTest)
    {
        auto& core_logger = Log::get_core_logger();
        auto& client_logger = Log::get_client_logger();

        EXPECT_NE(core_logger, nullptr);
        EXPECT_NE(client_logger, nullptr);
        EXPECT_EQ(core_logger->name(), "ENGINE");
        EXPECT_EQ(client_logger->name(), "APP");
    }

    TEST_F(LogTest, LogLevelsTest)
    {
        // Test that we can set log levels without crashing
        Log::set_level(spdlog::level::debug);
        Log::set_level(spdlog::level::info);
        Log::set_level(spdlog::level::warn);
        Log::set_level(spdlog::level::err);
    }

    TEST_F(LogTest, CoreLoggerMacros)
    {
        // These should not crash
        ENGINE_CORE_TRACE("This is a trace message");
        ENGINE_CORE_DEBUG("This is a debug message");
        ENGINE_CORE_INFO("This is an info message");
        ENGINE_CORE_WARN("This is a warning message");
        ENGINE_CORE_ERROR("This is an error message");
    }

    TEST_F(LogTest, ClientLoggerMacros)
    {
        // These should not crash
        ENGINE_TRACE("This is a trace message");
        ENGINE_DEBUG("This is a debug message");
        ENGINE_INFO("This is an info message");
        ENGINE_WARN("This is a warning message");
        ENGINE_ERROR("This is an error message");
    }

    TEST_F(LogTest, FormattedLogging)
    {
        // Test formatted logging
        int value = 42;
        std::string text = "test";

        ENGINE_CORE_INFO("Integer value: {}", value);
        ENGINE_CORE_INFO("String value: {}", text);
        ENGINE_CORE_INFO("Multiple values: {} and {}", value, text);
    }

    TEST_F(LogTest, CustomLoggerCreation)
    {
        auto custom_logger = Log::create_logger("CUSTOM");

        EXPECT_NE(custom_logger, nullptr);
        EXPECT_EQ(custom_logger->name(), "CUSTOM");

        custom_logger->info("Custom logger test message");
    }

} // namespace engine::core::tests

