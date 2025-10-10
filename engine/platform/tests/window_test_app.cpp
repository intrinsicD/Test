#include "../include/engine/platform/windowing/window.hpp"
#include "../include/engine/platform/windowing/window_console.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using namespace engine::platform;

std::optional<WindowBackend> parse_backend(std::string_view argument) {
    if (argument == "auto") {
        return WindowBackend::Auto;
    }
    if (argument == "mock") {
        return WindowBackend::Mock;
    }
    if (argument == "glfw") {
        return WindowBackend::GLFW;
    }
    if (argument == "sdl") {
        return WindowBackend::SDL;
    }
    return std::nullopt;
}

std::string build_usage_preamble(const char* executable) {
    std::ostringstream usage;
    usage << "Window test application\n"
          << "Usage: " << executable
          << " [--backend=<auto|mock|glfw|sdl>] [--title=<text>]"
          << " [--width=<pixels>] [--height=<pixels>] [--hidden]";
    return usage.str();
}

void print_full_usage(const char* executable) {
    std::cout << build_usage_preamble(executable) << '\n' << '\n';
    WindowConsole::print_command_reference(std::cout);
}

}  // namespace

int main(int argc, char** argv) {
    WindowConfig config{};
    WindowBackend backend = WindowBackend::Auto;

    for (int index = 1; index < argc; ++index) {
        std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            print_full_usage(argv[0]);
            return 0;
        }
        if (arg.rfind("--backend=", 0) == 0) {
            auto backend_name = arg.substr(std::string_view{"--backend="}.size());
            if (auto parsed = parse_backend(backend_name)) {
                backend = *parsed;
            } else {
                std::cerr << "Unknown backend: " << backend_name << '\n';
                return 1;
            }
            continue;
        }
        if (arg.rfind("--title=", 0) == 0) {
            config.title = std::string{arg.substr(std::string_view{"--title="}.size())};
            continue;
        }
        if (arg.rfind("--width=", 0) == 0) {
            config.width = static_cast<std::uint32_t>(std::stoul(
                std::string{arg.substr(std::string_view{"--width="}.size())}));
            continue;
        }
        if (arg.rfind("--height=", 0) == 0) {
            config.height = static_cast<std::uint32_t>(std::stoul(
                std::string{arg.substr(std::string_view{"--height="}.size())}));
            continue;
        }
        if (arg == "--hidden") {
            config.visible = false;
            continue;
        }

        std::cerr << "Unknown argument: " << arg << '\n';
        return 1;
    }

    std::shared_ptr<Window> window;
    try {
        window = create_window(config, backend);
    } catch (const std::exception& error) {
        std::cerr << "Failed to create window: " << error.what() << '\n';
        return 1;
    }

    if (!window) {
        std::cerr << "Failed to create window: returned nullptr" << '\n';
        return 1;
    }

    std::cout << "Created window using backend '" << window->backend_name() << "'\n";

    WindowConsoleOptions console_options{};
    console_options.usage_preamble = build_usage_preamble(argv[0]);

    WindowConsole console{*window, {}, console_options};
    console.print_status();
    std::cout << '\n';
    console.print_usage();
    std::cout << '\n';

    console.run();

    std::cout << "Exiting window test application" << '\n';
    return 0;
}

