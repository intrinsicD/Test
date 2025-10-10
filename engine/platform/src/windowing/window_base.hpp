#pragma once

#include "../../include/engine/platform/windowing/window.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace engine::platform::windowing {

class HeadlessSwapchainSurface final : public SwapchainSurface {
public:
    HeadlessSwapchainSurface(std::string renderer_backend,
                             std::string window_backend,
                             void* native_surface,
                             void* user_data) noexcept;

    [[nodiscard]] std::string_view renderer_backend() const noexcept override;
    [[nodiscard]] std::string_view window_backend() const noexcept override;
    [[nodiscard]] void* native_surface() const noexcept override;
    [[nodiscard]] void* user_data() const noexcept override;

private:
    std::string renderer_backend_;
    std::string window_backend_;
    void* native_surface_;
    void* user_data_;
};

class HeadlessWindow : public Window {
public:
    HeadlessWindow(std::string backend_name,
                   WindowConfig config,
                   std::shared_ptr<EventQueue> queue);
    ~HeadlessWindow() noexcept override;

    [[nodiscard]] std::string_view backend_name() const noexcept override;
    [[nodiscard]] const WindowConfig& config() const noexcept override;
    void show() override;
    void hide() override;
    [[nodiscard]] bool is_visible() const noexcept override;
    void request_close() override;
    [[nodiscard]] bool close_requested() const noexcept override;
    void post_event(Event event) override;
    void pump_events() override;
    [[nodiscard]] EventQueue& event_queue() noexcept override;
    [[nodiscard]] const EventQueue& event_queue() const noexcept override;
    [[nodiscard]] std::unique_ptr<SwapchainSurface> create_swapchain_surface(
        const SwapchainSurfaceRequest& request) override;

protected:
    [[nodiscard]] void* native_handle() noexcept;

private:
    void flush_pending_events();

    std::string backend_name_;
    WindowConfig config_;
    bool visible_;
    bool close_requested_;
    std::shared_ptr<EventQueue> queue_;
    std::deque<Event> pending_events_;
    std::mutex mutex_;
};

std::shared_ptr<Window> create_headless_window(std::string backend_name,
                                               WindowConfig config,
                                               std::shared_ptr<EventQueue> queue);

}  // namespace engine::platform::windowing

