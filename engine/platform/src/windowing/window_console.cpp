#include "../../include/engine/platform/windowing/window_console.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <utility>

namespace engine::platform {

namespace {

std::ostream& stream_or_sink(std::ostream* stream) {
    if (stream != nullptr) {
        return *stream;
    }
    static std::ostringstream sink;
    sink.str({});
    sink.clear();
    return sink;
}

}  // namespace

WindowConsole::WindowConsole(Window& window,
                             WindowConsoleStreams streams,
                             WindowConsoleOptions options)
    : window_{&window}, streams_{streams}, options_{std::move(options)} {
    if (streams_.input == nullptr) {
        streams_.input = &std::cin;
    }
    if (streams_.output == nullptr) {
        streams_.output = &std::cout;
    }
    if (streams_.error == nullptr) {
        streams_.error = &std::cerr;
    }
    if (options_.prompt.empty()) {
        options_.prompt = "> ";
    }
}

WindowConsole::~WindowConsole() noexcept {
    release_surface();
}

void WindowConsole::print_usage() const {
    auto& out = stream_or_sink(streams_.output);
    if (!options_.usage_preamble.empty()) {
        out << options_.usage_preamble << '\n';
    }
    print_command_reference(out);
}

void WindowConsole::print_status() const {
    if (window_ == nullptr) {
        return;
    }

    auto& out = stream_or_sink(streams_.output);
    const auto& config = window_->config();
    out << "[status] backend: " << window_->backend_name() << '\n';
    out << "[status] title: " << config.title << '\n';
    out << "[status] size: " << config.width << "x" << config.height << '\n';
    out << "[status] visible: " << (window_->is_visible() ? "true" : "false") << '\n';
    out << "[status] close requested: " << (window_->close_requested() ? "true" : "false") << '\n';
    out << "[status] queued events: " << window_->event_queue().size() << '\n';
}

void WindowConsole::print_event(const Event& event) const {
    auto& out = stream_or_sink(streams_.output);
    switch (event.type) {
        case EventType::None:
            out << "[event] none" << '\n';
            break;
        case EventType::CloseRequested:
            out << "[event] close requested" << '\n';
            break;
        case EventType::Resized: {
            if (const auto* payload = std::get_if<ResizeEvent>(&event.payload)) {
                out << "[event] resized to " << payload->width << "x" << payload->height << '\n';
            } else {
                out << "[event] resized (missing payload)" << '\n';
            }
            break;
        }
        case EventType::FocusChanged: {
            if (const auto* payload = std::get_if<FocusEvent>(&event.payload)) {
                out << "[event] focus changed -> " << (payload->focused ? "focused" : "unfocused")
                    << '\n';
            } else {
                out << "[event] focus changed (missing payload)" << '\n';
            }
            break;
        }
        case EventType::Custom: {
            if (const auto* payload = std::get_if<std::string>(&event.payload)) {
                out << "[event] custom: " << *payload << '\n';
            } else {
                out << "[event] custom (missing payload)" << '\n';
            }
            break;
        }
    }
}

void WindowConsole::drain_events(bool pump_backend) {
    if (window_ == nullptr) {
        return;
    }

    if (pump_backend) {
        window_->pump_events();
    }

    auto& queue = window_->event_queue();
    Event event;
    bool any = false;
    while (queue.poll(event)) {
        print_event(event);
        any = true;
    }

    if (!any) {
        stream_or_sink(streams_.output) << "[event] queue empty" << '\n';
    }
}

bool WindowConsole::handle_command_line(std::string line_text) {
    if (window_ == nullptr) {
        return false;
    }

    line_text = trim_leading_whitespace(std::move(line_text));
    if (line_text.empty()) {
        return true;
    }

    std::istringstream stream{line_text};
    std::string command;
    stream >> command;

    auto& out = stream_or_sink(streams_.output);

    if (command == "help") {
        print_usage();
        return true;
    }
    if (command == "quit" || command == "exit") {
        return false;
    }
    if (command == "show") {
        window_->show();
        out << "[action] show requested" << '\n';
        return true;
    }
    if (command == "hide") {
        window_->hide();
        out << "[action] hide requested" << '\n';
        return true;
    }
    if (command == "status") {
        print_status();
        return true;
    }
    if (command == "request-close") {
        window_->request_close();
        out << "[action] close requested flag set" << '\n';
        return true;
    }
    if (command == "pump") {
        window_->pump_events();
        out << "[action] pumped backend events" << '\n';
        return true;
    }
    if (command == "poll") {
        Event event;
        if (window_->event_queue().poll(event)) {
            print_event(event);
        } else {
            out << "[event] queue empty" << '\n';
        }
        return true;
    }
    if (command == "drain") {
        drain_events(true);
        return true;
    }
    if (command == "surface") {
        std::string renderer;
        stream >> renderer;
        if (renderer.empty()) {
            out << "Usage: surface <renderer>" << '\n';
            return true;
        }
        last_surface_ = create_surface_with_logging(std::move(renderer));
        return true;
    }
    if (command == "post") {
        std::string type;
        stream >> type;
        if (type.empty()) {
            out << "Usage: post <close|resize|focus|custom>" << '\n';
            return true;
        }

        if (type == "close") {
            window_->post_event(Event::close_requested());
            out << "[action] queued synthetic close event" << '\n';
            return true;
        }
        if (type == "resize") {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            stream >> width >> height;
            if (stream.fail()) {
                out << "Usage: post resize <width> <height>" << '\n';
                return true;
            }
            window_->post_event(Event::resized(width, height));
            out << "[action] queued synthetic resize event" << '\n';
            return true;
        }
        if (type == "focus") {
            int focus_value = 0;
            stream >> focus_value;
            if (stream.fail() || (focus_value != 0 && focus_value != 1)) {
                out << "Usage: post focus <0|1>" << '\n';
                return true;
            }
            window_->post_event(Event::focus_changed(focus_value == 1));
            out << "[action] queued synthetic focus event" << '\n';
            return true;
        }
        if (type == "custom") {
            std::string payload;
            std::getline(stream, payload);
            payload = trim_leading_whitespace(std::move(payload));
            window_->post_event(Event::custom(std::move(payload)));
            out << "[action] queued synthetic custom event" << '\n';
            return true;
        }

        out << "Unknown synthetic event type: " << type << '\n';
        return true;
    }

    out << "Unknown command: " << command << '\n';
    out << "Type 'help' to list available commands." << '\n';
    return true;
}

void WindowConsole::run() {
    if (window_ == nullptr || streams_.input == nullptr) {
        return;
    }

    std::string line_buffer;
    while (true) {
        auto& out = stream_or_sink(streams_.output);
        out << options_.prompt;
        out.flush();

        if (!std::getline(*streams_.input, line_buffer)) {
            break;
        }

        if (!handle_command_line(line_buffer)) {
            break;
        }
    }

    release_surface();
}

std::string WindowConsole::trim_leading_whitespace(std::string text) {
    auto it = text.begin();
    while (it != text.end() && std::isspace(static_cast<unsigned char>(*it)) != 0) {
        ++it;
    }
    text.erase(text.begin(), it);
    return text;
}

void WindowConsole::release_surface() {
    if (!last_surface_) {
        return;
    }
    stream_or_sink(streams_.output) << "[surface] releasing last created surface" << '\n';
    last_surface_.reset();
}

std::unique_ptr<SwapchainSurface> WindowConsole::create_surface_with_logging(std::string renderer_backend) {
    if (window_ == nullptr) {
        return nullptr;
    }

    SwapchainSurfaceRequest request{};
    request.renderer_backend = std::move(renderer_backend);
    request.user_data = nullptr;
    request.hook = [&](const SwapchainSurfaceRequest& req, void* native) {
        auto& out = stream_or_sink(streams_.output);
        out << "[surface] hook invoked for renderer '" << req.renderer_backend
            << "' and backend '" << window_->backend_name() << "'" << '\n';
        out << "            native handle: " << native << '\n';
        return std::unique_ptr<SwapchainSurface>{};
    };

    auto surface = window_->create_swapchain_surface(request);
    auto& out = stream_or_sink(streams_.output);
    if (surface) {
        out << "[surface] renderer: " << surface->renderer_backend() << '\n';
        out << "[surface] window backend: " << surface->window_backend() << '\n';
        out << "[surface] native handle: " << surface->native_surface() << '\n';
    } else {
        out << "[surface] creation failed" << '\n';
    }

    return surface;
}

void WindowConsole::print_command_reference(std::ostream& output) {
    output << "Interactive commands (type and press enter):\n"
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

}  // namespace engine::platform

