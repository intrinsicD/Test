#pragma once

#include "window.hpp"

#include <iosfwd>
#include <iostream>
#include <memory>
#include <string>

namespace engine::platform {

/// \brief Configures the input and output streams consumed by WindowConsole.
struct WindowConsoleStreams {
    std::istream* input{&std::cin};
    std::ostream* output{&std::cout};
    std::ostream* error{&std::cerr};
};

/// \brief Additional runtime configuration for WindowConsole.
struct WindowConsoleOptions {
    std::string prompt{"> "};
    std::string usage_preamble{};
};

/// \brief Implements an interactive command console around a Window instance.
class WindowConsole {
public:
    /// \brief Constructs the console bound to a live window instance.
    WindowConsole(Window& window,
                  WindowConsoleStreams streams = {},
                  WindowConsoleOptions options = {});
    ~WindowConsole() noexcept;

    WindowConsole(const WindowConsole&) = delete;
    WindowConsole& operator=(const WindowConsole&) = delete;
    WindowConsole(WindowConsole&&) = delete;
    WindowConsole& operator=(WindowConsole&&) = delete;

    /// \brief Prints the interactive usage text to the configured output stream.
    void print_usage() const;

    /// \brief Emits a diagnostic snapshot of the bound window state.
    void print_status() const;

    /// \brief Drains the pending event queue and optionally pumps new backend events.
    void drain_events(bool pump_backend);

    /// \brief Runs the interactive loop until quit, exit, or EOF is encountered.
    void run();

    /// \brief Handles a single command line. Returns false when the session should terminate.
    bool handle_command_line(std::string line);

    /// \brief Prints a single event to the configured output stream.
    void print_event(const Event& event) const;

    /// \brief Emits the interactive command reference to \p output.
    static void print_command_reference(std::ostream& output);

private:
    static std::string trim_leading_whitespace(std::string text);

    void release_surface();
    std::unique_ptr<SwapchainSurface> create_surface_with_logging(std::string renderer_backend);

    Window* window_{nullptr};
    WindowConsoleStreams streams_{};
    WindowConsoleOptions options_{};
    std::unique_ptr<SwapchainSurface> last_surface_{};
};

}  // namespace engine::platform

