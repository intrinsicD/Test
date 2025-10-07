#include "engine/platform/window.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

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
    using windowing::create_glfw_window;
    using windowing::create_mock_window;
    using windowing::create_sdl_window;

    switch (backend) {
        case WindowBackend::Auto:
        case WindowBackend::Mock:
            return create_mock_window(std::move(config), std::move(queue));
        case WindowBackend::GLFW:
            return create_glfw_window(std::move(config), std::move(queue));
        case WindowBackend::SDL:
            return create_sdl_window(std::move(config), std::move(queue));
    }

    throw std::runtime_error{"Unsupported window backend"};
}

}  // namespace

std::shared_ptr<EventQueue> create_event_queue() {
    return std::make_shared<LocalEventQueue>();
}

std::shared_ptr<Window> create_window(WindowConfig config,
                                      WindowBackend backend,
                                      std::shared_ptr<EventQueue> event_queue) {
    auto queue = ensure_queue(std::move(event_queue));
    if (backend == WindowBackend::Auto) {
        backend = WindowBackend::Mock;
    }

    return create_window_with_backend(std::move(config), backend, std::move(queue));
}

}  // namespace engine::platform

