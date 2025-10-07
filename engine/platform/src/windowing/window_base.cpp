#include "window_base.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace engine::platform::windowing {

HeadlessSwapchainSurface::HeadlessSwapchainSurface(std::string renderer_backend,
                                                   std::string window_backend,
                                                   void* native_surface,
                                                   void* user_data) noexcept
    : renderer_backend_(std::move(renderer_backend)),
      window_backend_(std::move(window_backend)),
      native_surface_(native_surface),
      user_data_(user_data) {}

std::string_view HeadlessSwapchainSurface::renderer_backend() const noexcept {
    return renderer_backend_;
}

std::string_view HeadlessSwapchainSurface::window_backend() const noexcept {
    return window_backend_;
}

void* HeadlessSwapchainSurface::native_surface() const noexcept {
    return native_surface_;
}

void* HeadlessSwapchainSurface::user_data() const noexcept {
    return user_data_;
}

HeadlessWindow::HeadlessWindow(std::string backend_name,
                               WindowConfig config,
                               std::shared_ptr<EventQueue> queue)
    : backend_name_(std::move(backend_name)),
      config_(std::move(config)),
      visible_(config_.visible),
      close_requested_(false),
      queue_(std::move(queue)) {
    if (!queue_) {
        throw std::invalid_argument{"Event queue must not be null"};
    }
}

HeadlessWindow::~HeadlessWindow() noexcept = default;

std::string_view HeadlessWindow::backend_name() const noexcept {
    return backend_name_;
}

const WindowConfig& HeadlessWindow::config() const noexcept {
    return config_;
}

void HeadlessWindow::show() {
    visible_ = true;
}

void HeadlessWindow::hide() {
    visible_ = false;
}

bool HeadlessWindow::is_visible() const noexcept {
    return visible_;
}

void HeadlessWindow::request_close() {
    close_requested_ = true;
    post_event(Event::close_requested());
}

bool HeadlessWindow::close_requested() const noexcept {
    return close_requested_;
}

void HeadlessWindow::post_event(Event event) {
    std::scoped_lock lock{mutex_};
    pending_events_.push_back(std::move(event));
}

void HeadlessWindow::pump_events() {
    flush_pending_events();
    close_requested_ = false;
}

EventQueue& HeadlessWindow::event_queue() noexcept {
    return *queue_;
}

const EventQueue& HeadlessWindow::event_queue() const noexcept {
    return *queue_;
}

std::unique_ptr<SwapchainSurface> HeadlessWindow::create_swapchain_surface(
    const SwapchainSurfaceRequest& request) {
    if (request.hook) {
        if (auto surface = request.hook(request, native_handle())) {
            return surface;
        }
    }

    return std::make_unique<HeadlessSwapchainSurface>(
        request.renderer_backend,
        std::string{backend_name_},
        native_handle(),
        request.user_data);
}

void* HeadlessWindow::native_handle() noexcept {
    return this;
}

void HeadlessWindow::flush_pending_events() {
    std::deque<Event> local;
    {
        std::scoped_lock lock{mutex_};
        local.swap(pending_events_);
    }

    for (auto& event : local) {
        queue_->push(std::move(event));
    }
}

std::shared_ptr<Window> create_headless_window(std::string backend_name,
                                               WindowConfig config,
                                               std::shared_ptr<EventQueue> queue) {
    return std::make_shared<HeadlessWindow>(
        std::move(backend_name),
        std::move(config),
        std::move(queue));
}

}  // namespace engine::platform::windowing

