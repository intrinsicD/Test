#include "engine/platform/windowing/window.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::platform {

namespace windowing {
std::shared_ptr<Window> create_mock_window(WindowConfig config,
                                           std::shared_ptr<EventQueue> queue);
std::shared_ptr<Window> create_glfw_window(WindowConfig config,
                                           std::shared_ptr<EventQueue> queue);
std::shared_ptr<Window> create_sdl_window(WindowConfig config,
                                          std::shared_ptr<EventQueue> queue);
}  // namespace windowing

namespace {

#ifndef ENGINE_PLATFORM_DEFAULT_BACKEND
#define ENGINE_PLATFORM_DEFAULT_BACKEND "GLFW"
#endif

constexpr std::string_view kBackendEnvVar = "ENGINE_PLATFORM_WINDOW_BACKEND";
constexpr std::array<WindowBackend, 3> kDefaultBackendOrder = {
    WindowBackend::GLFW,
    WindowBackend::SDL,
    WindowBackend::Mock,
};

using WindowFactory = std::shared_ptr<Window> (*)(WindowConfig, std::shared_ptr<EventQueue>);

struct BackendDescriptor {
    WindowBackend backend;
    WindowBackendCapabilities capabilities;
    WindowFactory factory;
};

constexpr std::array<BackendDescriptor, 3> kBackendDescriptors = {{
    {WindowBackend::Mock, WindowBackendCapabilities{true, false}, windowing::create_mock_window},
    {WindowBackend::GLFW, WindowBackendCapabilities{false, true}, windowing::create_glfw_window},
    {WindowBackend::SDL, WindowBackendCapabilities{true, true}, windowing::create_sdl_window},
}};

[[nodiscard]] const BackendDescriptor* find_descriptor(WindowBackend backend) noexcept {
    for (const auto& descriptor : kBackendDescriptors) {
        if (descriptor.backend == backend) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] const char* backend_identifier(WindowBackend backend) noexcept {
    switch (backend) {
        case WindowBackend::Auto:
            return "auto";
        case WindowBackend::GLFW:
            return "glfw";
        case WindowBackend::SDL:
            return "sdl";
        case WindowBackend::Mock:
            return "mock";
    }

    return "unknown";
}

[[nodiscard]] std::optional<std::string> getenv_string(std::string_view name) {
    const auto environment_key = std::string{name};
    if (const char* value = std::getenv(environment_key.c_str()); value != nullptr && value[0] != '\0') {
        return std::string{value};
    }
    return std::nullopt;
}

[[nodiscard]] std::string normalise_backend_override(std::string_view value) {
    const auto* begin = value.data();
    const auto* end = begin + value.size();

    while (begin != end && std::isspace(static_cast<unsigned char>(*begin)) != 0) {
        ++begin;
    }
    while (begin != end && std::isspace(static_cast<unsigned char>(*(end - 1))) != 0) {
        --end;
    }

    std::string normalised;
    normalised.reserve(static_cast<std::size_t>(end - begin));
    for (const char* it = begin; it != end; ++it) {
        normalised.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*it))));
    }
    return normalised;
}

[[nodiscard]] std::optional<WindowBackend> parse_backend_override(const std::string& value) {
    const auto normalised = normalise_backend_override(value);

    if (normalised == "auto") {
        return WindowBackend::Auto;
    }
    if (normalised == "mock") {
        return WindowBackend::Mock;
    }
    if (normalised == "glfw") {
        return WindowBackend::GLFW;
    }
    if (normalised == "sdl") {
        return WindowBackend::SDL;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<WindowBackend> read_backend_override() {
    if (auto value = getenv_string(kBackendEnvVar)) {
        if (auto parsed = parse_backend_override(*value)) {
            return parsed;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<WindowBackend> configured_backend() {
    constexpr std::string_view configured = ENGINE_PLATFORM_DEFAULT_BACKEND;
    if (configured.empty()) {
        return std::nullopt;
    }

    if (auto parsed = parse_backend_override(std::string{configured})) {
        if (*parsed == WindowBackend::Auto) {
            return std::nullopt;
        }
        return parsed;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> capability_violation(
    WindowBackend backend,
    const WindowConfig& config,
    const BackendDescriptor& descriptor) {
    std::vector<std::string_view> missing;
    if (config.requires_headless_safe() && !descriptor.capabilities.headless_safe) {
        missing.emplace_back("headless_safe");
    }
    if (config.requires_native_surface() && !descriptor.capabilities.native_surface) {
        missing.emplace_back("native_surface");
    }

    if (missing.empty()) {
        return std::nullopt;
    }

    std::ostringstream builder;
    builder << "backend '" << backend_identifier(backend)
            << "' does not satisfy capability requirements (missing ";
    for (std::size_t index = 0; index < missing.size(); ++index) {
        if (index != 0) {
            builder << ", ";
        }
        builder << missing[index];
    }
    builder << ')';
    return builder.str();
}

[[nodiscard]] std::vector<WindowBackend> build_candidate_backends(
    const WindowConfig& config,
    std::optional<WindowBackend> override_backend) {
    std::vector<WindowBackend> candidates;
    candidates.reserve(kBackendDescriptors.size() + 1);

    auto append_candidate = [&candidates, &config](WindowBackend backend) {
        if (backend == WindowBackend::Auto) {
            return;
        }
        if (std::find(candidates.begin(), candidates.end(), backend) != candidates.end()) {
            return;
        }
        if (const auto* descriptor = find_descriptor(backend)) {
            if (!capability_violation(backend, config, *descriptor)) {
                candidates.push_back(backend);
            }
        }
    };

    if (override_backend && *override_backend != WindowBackend::Auto) {
        if (const auto* descriptor = find_descriptor(*override_backend)) {
            if (auto violation = capability_violation(*override_backend, config, *descriptor)) {
                throw std::runtime_error{*violation};
            }
            candidates.push_back(*override_backend);
        } else {
            std::ostringstream builder;
            builder << "Unknown window backend override '"
                    << backend_identifier(*override_backend) << "'";
            throw std::runtime_error{builder.str()};
        }
    }

    if (auto build_default = configured_backend()) {
        append_candidate(*build_default);
    }

    for (auto backend_option : kDefaultBackendOrder) {
        append_candidate(backend_option);
    }

    return candidates;
}

class LocalEventQueue final : public EventQueue {
public:
    void push(Event event) override {
        std::scoped_lock lock{mutex_};
        queue_.push_back(std::move(event));
    }

    bool poll(Event& out_event) override {
        std::scoped_lock lock{mutex_};
        if (queue_.empty()) {
            return false;
        }

        out_event = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void clear() noexcept override {
        std::scoped_lock lock{mutex_};
        queue_.clear();
    }

    [[nodiscard]] bool empty() const noexcept override {
        std::scoped_lock lock{mutex_};
        return queue_.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept override {
        std::scoped_lock lock{mutex_};
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::deque<Event> queue_;
};

std::shared_ptr<EventQueue> ensure_queue(std::shared_ptr<EventQueue> queue) {
    if (!queue) {
        return std::make_shared<LocalEventQueue>();
    }
    return queue;
}

std::shared_ptr<Window> create_window_with_backend(WindowConfig config,
                                                   WindowBackend backend,
                                                   std::shared_ptr<EventQueue> queue) {
    if (backend == WindowBackend::Auto) {
        throw std::runtime_error{"Automatic backend selection is handled by create_window"};
    }

    const auto* descriptor = find_descriptor(backend);
    if (descriptor == nullptr) {
        throw std::runtime_error{"Unsupported window backend"};
    }

    if (auto violation = capability_violation(backend, config, *descriptor)) {
        throw std::runtime_error{*violation};
    }

    return descriptor->factory(std::move(config), std::move(queue));
}

}  // namespace

std::shared_ptr<EventQueue> create_event_queue() {
    return std::make_shared<LocalEventQueue>();
}

std::shared_ptr<Window> create_window(WindowConfig config,
                                      WindowBackend backend,
                                      std::shared_ptr<EventQueue> event_queue) {
    auto queue = ensure_queue(std::move(event_queue));
    if (backend != WindowBackend::Auto) {
        return create_window_with_backend(std::move(config), backend, std::move(queue));
    }

    const auto override_backend = read_backend_override();
    const auto candidates = build_candidate_backends(config, override_backend);

    std::vector<std::string> errors;
    errors.reserve(candidates.size());

    for (auto candidate : candidates) {
        try {
            return create_window_with_backend(config, candidate, queue);
        } catch (const std::exception& error) {
            std::ostringstream builder;
            builder << backend_identifier(candidate) << ": " << error.what();
            errors.push_back(builder.str());
        } catch (...) {
            std::ostringstream builder;
            builder << backend_identifier(candidate) << ": unknown error";
            errors.push_back(builder.str());
        }
    }

    std::ostringstream message;
    message << "Automatic backend selection failed";
    if (!errors.empty()) {
        message << " (";
        for (std::size_t index = 0; index < errors.size(); ++index) {
            if (index != 0) {
                message << "; ";
            }
            message << errors[index];
        }
        message << ')';
    }

    throw std::runtime_error{message.str()};
}

}  // namespace engine::platform

