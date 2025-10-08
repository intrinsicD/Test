#include "engine/platform/window.hpp"

#include <cctype>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

using namespace engine::platform;

//------------------------------------------------------------------------------
// Utility helpers
//------------------------------------------------------------------------------

[[nodiscard]] std::string trim_leading_whitespace(std::string text) {
    auto it = text.begin();
    while (it != text.end() && std::isspace(static_cast<unsigned char>(*it)) != 0) {
        ++it;
    }
    text.erase(text.begin(), it);
    return text;
}

[[nodiscard]] std::optional<WindowBackend> parse_backend(std::string_view argument) {
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

void print_usage(const char* executable) {
    std::cout << "Window test application\n"
              << "Usage: " << executable << " [--backend=<auto|mock|glfw|sdl>]"
              << " [--title=<text>] [--width=<pixels>] [--height=<pixels>]"
              << " [--hidden]" << '\n';
    std::cout << '\n'
              << "Interactive commands (type and press enter):\n"
              << "  help                Show the interactive command list\n"
              << "  status              Print current window state\n"
              << "  show | hide         Toggle the requested visibility\n"
              << "  request-close       Ask the backend to close the window\n"
              << "  post <event...>     Queue a synthetic event (see below)\n"
              << "  pump                Pump backend events\n"
              << "  poll                Poll and print a single event\n"
              << "  drain               Pump and print all pending events\n"
              << "  surface <renderer>  Create a swapchain surface for testing\n"
              << "  quit                Exit the application\n"
              << '\n'
              << "Synthetic events:\n"
              << "  post close\n"
              << "  post resize <width> <height>\n"
              << "  post focus <0|1>\n"
              << "  post custom <message>\n";
}

void print_event(const Event& event) {
    switch (event.type) {
        case EventType::None:
            std::cout << "[event] none" << '\n';
            break;
        case EventType::CloseRequested:
            std::cout << "[event] close requested" << '\n';
            break;
        case EventType::Resized: {
            if (const auto* payload = std::get_if<ResizeEvent>(&event.payload)) {
                std::cout << "[event] resized to " << payload->width << "x" << payload->height << '\n';
            } else {
                std::cout << "[event] resized (missing payload)" << '\n';
            }
            break;
        }
        case EventType::FocusChanged: {
            if (const auto* payload = std::get_if<FocusEvent>(&event.payload)) {
                std::cout << "[event] focus changed -> " << (payload->focused ? "focused" : "unfocused") << '\n';
            } else {
                std::cout << "[event] focus changed (missing payload)" << '\n';
            }
            break;
        }
        case EventType::Custom: {
            if (const auto* payload = std::get_if<std::string>(&event.payload)) {
                std::cout << "[event] custom: " << *payload << '\n';
            } else {
                std::cout << "[event] custom (missing payload)" << '\n';
            }
            break;
        }
    }
}

void drain_events(Window& window, bool pump_backend) {
    if (pump_backend) {
        window.pump_events();
    }

    auto& queue = window.event_queue();
    Event event;
    bool any = false;
    while (queue.poll(event)) {
        print_event(event);
        any = true;
    }

    if (!any) {
        std::cout << "[event] queue empty" << '\n';
    }
}

std::unique_ptr<SwapchainSurface> create_surface_with_logging(Window& window,
                                                              std::string renderer_backend) {
    SwapchainSurfaceRequest request{};
    request.renderer_backend = std::move(renderer_backend);
    request.user_data = nullptr;
    request.hook = [&](const SwapchainSurfaceRequest& req, void* native) {
        std::cout << "[surface] hook invoked for renderer '" << req.renderer_backend
                  << "' and backend '" << window.backend_name() << "'" << '\n';
        std::cout << "            native handle: " << native << '\n';
        return std::unique_ptr<SwapchainSurface>{};
    };

    auto surface = window.create_swapchain_surface(request);
    if (surface) {
        std::cout << "[surface] renderer: " << surface->renderer_backend() << '\n';
        std::cout << "[surface] window backend: " << surface->window_backend() << '\n';
        std::cout << "[surface] native handle: " << surface->native_surface() << '\n';
    } else {
        std::cout << "[surface] creation failed" << '\n';
    }

    return surface;
}

void print_status(const Window& window) {
    const auto& config = window.config();
    std::cout << "[status] backend: " << window.backend_name() << '\n';
    std::cout << "[status] title: " << config.title << '\n';
    std::cout << "[status] size: " << config.width << "x" << config.height << '\n';
    std::cout << "[status] visible: " << (window.is_visible() ? "true" : "false") << '\n';
    std::cout << "[status] close requested: " << (window.close_requested() ? "true" : "false")
              << '\n';
    std::cout << "[status] queued events: " << window.event_queue().size() << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    WindowConfig config{};
    WindowBackend backend = WindowBackend::Auto;

    for (int index = 1; index < argc; ++index) {
        std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
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
            config.width = static_cast<std::uint32_t>(std::stoul(std::string{arg.substr(std::string_view{"--width="}.size())}));
            continue;
        }
        if (arg.rfind("--height=", 0) == 0) {
            config.height = static_cast<std::uint32_t>(std::stoul(std::string{arg.substr(std::string_view{"--height="}.size())}));
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
    print_status(*window);
    std::cout << '\n';
    print_usage(argv[0]);
    std::cout << '\n';

    std::unique_ptr<SwapchainSurface> last_surface;

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }

        line = trim_leading_whitespace(std::move(line));
        if (line.empty()) {
            continue;
        }

        std::istringstream stream{line};
        std::string command;
        stream >> command;

        if (command == "help") {
            print_usage(argv[0]);
            continue;
        }
        if (command == "quit" || command == "exit") {
            break;
        }
        if (command == "show") {
            window->show();
            std::cout << "[action] show requested" << '\n';
            continue;
        }
        if (command == "hide") {
            window->hide();
            std::cout << "[action] hide requested" << '\n';
            continue;
        }
        if (command == "status") {
            print_status(*window);
            continue;
        }
        if (command == "request-close") {
            window->request_close();
            std::cout << "[action] close requested flag set" << '\n';
            continue;
        }
        if (command == "pump") {
            window->pump_events();
            std::cout << "[action] pumped backend events" << '\n';
            continue;
        }
        if (command == "poll") {
            Event event;
            if (window->event_queue().poll(event)) {
                print_event(event);
            } else {
                std::cout << "[event] queue empty" << '\n';
            }
            continue;
        }
        if (command == "drain") {
            drain_events(*window, true);
            continue;
        }
        if (command == "surface") {
            std::string renderer;
            stream >> renderer;
            if (renderer.empty()) {
                std::cout << "Usage: surface <renderer>" << '\n';
                continue;
            }
            last_surface = create_surface_with_logging(*window, renderer);
            continue;
        }
        if (command == "post") {
            std::string type;
            stream >> type;
            if (type.empty()) {
                std::cout << "Usage: post <close|resize|focus|custom>" << '\n';
                continue;
            }

            if (type == "close") {
                window->post_event(Event::close_requested());
                std::cout << "[action] queued synthetic close event" << '\n';
                continue;
            }
            if (type == "resize") {
                std::uint32_t width = 0;
                std::uint32_t height = 0;
                stream >> width >> height;
                if (stream.fail()) {
                    std::cout << "Usage: post resize <width> <height>" << '\n';
                    continue;
                }
                window->post_event(Event::resized(width, height));
                std::cout << "[action] queued synthetic resize event" << '\n';
                continue;
            }
            if (type == "focus") {
                int focus_value = 0;
                stream >> focus_value;
                if (stream.fail() || (focus_value != 0 && focus_value != 1)) {
                    std::cout << "Usage: post focus <0|1>" << '\n';
                    continue;
                }
                window->post_event(Event::focus_changed(focus_value == 1));
                std::cout << "[action] queued synthetic focus event" << '\n';
                continue;
            }
            if (type == "custom") {
                std::string payload;
                std::getline(stream, payload);
                payload = trim_leading_whitespace(std::move(payload));
                window->post_event(Event::custom(std::move(payload)));
                std::cout << "[action] queued synthetic custom event" << '\n';
                continue;
            }

            std::cout << "Unknown synthetic event type: " << type << '\n';
            continue;
        }

        std::cout << "Unknown command: " << command << '\n';
        std::cout << "Type 'help' to list available commands." << '\n';
    }

    if (last_surface) {
        std::cout << "[surface] releasing last created surface" << '\n';
        last_surface.reset();
    }

    std::cout << "Exiting window test application" << '\n';
    return 0;
}
