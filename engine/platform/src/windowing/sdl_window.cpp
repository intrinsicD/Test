#include "window_base.hpp"

#include <cstddef>
#include <mutex>
#include <utility>

namespace engine::platform::windowing {
namespace {

class SdlStubLibrary {
public:
    static SdlStubLibrary& instance() {
        static SdlStubLibrary library;
        return library;
    }

    void retain() {
        std::scoped_lock lock{mutex_};
        ++ref_count_;
    }

    void release() noexcept {
        std::scoped_lock lock{mutex_};
        if (ref_count_ > 0) {
            --ref_count_;
        }
    }

    void notify_visibility_change(bool visible) noexcept {
        std::scoped_lock lock{mutex_};
        last_visible_ = visible;
    }

    void notify_close_request() noexcept {
        std::scoped_lock lock{mutex_};
        ++close_requests_;
    }

    void pump() noexcept {
        std::scoped_lock lock{mutex_};
        // No-op stub mirroring SDL_PumpEvents semantics.
        (void)last_visible_;
        (void)close_requests_;
    }

private:
    SdlStubLibrary() = default;

    mutable std::mutex mutex_;
    std::size_t ref_count_{0};
    bool last_visible_{false};
    std::size_t close_requests_{0};
};

class SdlWindow final : public HeadlessWindow {
public:
    SdlWindow(WindowConfig config, std::shared_ptr<EventQueue> queue)
        : HeadlessWindow("sdl", std::move(config), std::move(queue)) {
        SdlStubLibrary::instance().retain();
    }

    ~SdlWindow() noexcept override {
        SdlStubLibrary::instance().release();
    }

    void show() override {
        HeadlessWindow::show();
        SdlStubLibrary::instance().notify_visibility_change(true);
    }

    void hide() override {
        HeadlessWindow::hide();
        SdlStubLibrary::instance().notify_visibility_change(false);
    }

    void request_close() override {
        HeadlessWindow::request_close();
        SdlStubLibrary::instance().notify_close_request();
    }

    void pump_events() override {
        SdlStubLibrary::instance().pump();
        HeadlessWindow::pump_events();
    }
};

}  // namespace

std::shared_ptr<Window> create_sdl_window(WindowConfig config,
                                          std::shared_ptr<EventQueue> queue) {
    return std::make_shared<SdlWindow>(std::move(config), std::move(queue));
}

}  // namespace engine::platform::windowing

