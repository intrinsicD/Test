#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "../include/engine/platform/windowing/window.hpp"
#include "../include/engine/platform/windowing/window_console.hpp"

namespace {

using namespace engine::platform;

TEST(WindowConsole, HandlesCommandSequence) {
    auto window = create_window(WindowConfig{}, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);

    std::istringstream input{"status\npost custom payload\npump\npoll\nquit\n"};
    std::ostringstream output;
    std::ostringstream error;

    WindowConsoleStreams streams{};
    streams.input = &input;
    streams.output = &output;
    streams.error = &error;

    WindowConsole console{*window, streams};
    console.run();

    const std::string text = output.str();
    EXPECT_TRUE(text.find("[status] backend") != std::string::npos);
    EXPECT_TRUE(text.find("[action] queued synthetic custom event") != std::string::npos);
    EXPECT_TRUE(text.find("[event] custom: payload") != std::string::npos);
    EXPECT_TRUE(window->event_queue().empty());
}

TEST(WindowConsole, SurfaceCommandLogsLifecycle) {
    auto window = create_window(WindowConfig{}, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);

    std::istringstream input{"surface renderer\nquit\n"};
    std::ostringstream output;

    WindowConsoleStreams streams{};
    streams.input = &input;
    streams.output = &output;

    WindowConsole console{*window, streams};
    console.run();

    const std::string text = output.str();
    EXPECT_TRUE(text.find("[surface] hook invoked for renderer 'renderer'") != std::string::npos);
    EXPECT_TRUE(text.find("[surface] renderer: renderer") != std::string::npos);
    EXPECT_TRUE(text.find("[surface] releasing last created surface") != std::string::npos);
}

TEST(WindowConsole, UnknownCommandDisplaysHelpHint) {
    auto window = create_window(WindowConfig{}, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);

    std::istringstream input{"bogus\nquit\n"};
    std::ostringstream output;

    WindowConsoleStreams streams{};
    streams.input = &input;
    streams.output = &output;

    WindowConsole console{*window, streams};
    console.run();

    const std::string text = output.str();
    EXPECT_TRUE(text.find("Unknown command: bogus") != std::string::npos);
    EXPECT_TRUE(text.find("Type 'help' to list available commands.") != std::string::npos);
}

}  // namespace

