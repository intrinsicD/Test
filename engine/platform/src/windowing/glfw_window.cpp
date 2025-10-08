#include "window_base.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace engine::platform::windowing {
namespace {

class GlfwLibrary {
public:
    static GlfwLibrary& instance() {
        static GlfwLibrary library;
        return library;
    }

    void retain() {
        std::unique_lock lock{mutex_};
        wait_for_initialisation(lock);

        if (ref_count_ > 0) {
            ++ref_count_;
            return;
        }

        initialising_ = true;
        last_error_.clear();
        lock.unlock();

        glfwSetErrorCallback(&GlfwLibrary::handle_error);
        const int result = glfwInit();

        lock.lock();
        initialising_ = false;
        if (result != GLFW_TRUE) {
            const std::string message = last_error_;
            ready_.notify_all();
            lock.unlock();
            init_failed(message);
        }

        ref_count_ = 1;
        ready_.notify_all();
        lock.unlock();
    }

    void release() noexcept {
        std::unique_lock lock{mutex_};
        if (ref_count_ == 0) {
            return;
        }

        --ref_count_;
        if (ref_count_ == 0) {
            lock.unlock();
            glfwTerminate();
            glfwSetErrorCallback(nullptr);
        }
    }

    void record_error(int code, const char* description) noexcept {
        std::scoped_lock lock{mutex_};
        std::ostringstream builder;
        builder << "GLFW error " << code;
        if (description != nullptr && description[0] != '\0') {
            builder << ": " << description;
        }
        last_error_ = builder.str();
    }

    [[nodiscard]] std::string last_error() const {
        std::scoped_lock lock{mutex_};
        return last_error_;
    }

private:
    GlfwLibrary() = default;

    static void handle_error(int code, const char* description) noexcept {
        GlfwLibrary::instance().record_error(code, description);
    }

    void wait_for_initialisation(std::unique_lock<std::mutex>& lock) {
        ready_.wait(lock, [this] { return !initialising_; });
    }

    [[noreturn]] void init_failed(const std::string& message) {
        glfwSetErrorCallback(nullptr);
        if (message.empty()) {
            throw std::runtime_error{"Failed to initialise GLFW"};
        }
        throw std::runtime_error{"Failed to initialise GLFW: " + message};
    }

    mutable std::mutex mutex_;
    std::condition_variable ready_;
    std::size_t ref_count_{0};
    bool initialising_{false};
    std::string last_error_;
};

class GlfwWindow final : public HeadlessWindow {
public:
    GlfwWindow(WindowConfig config, std::shared_ptr<EventQueue> queue)
        : HeadlessWindow("glfw", std::move(config), std::move(queue)) {
        auto& library = GlfwLibrary::instance();
        library.retain();

        try {
            create_window();
        } catch (...) {
            library.release();
            throw;
        }
    }

    ~GlfwWindow() noexcept override {
        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }

        GlfwLibrary::instance().release();
    }

    void show() override {
        HeadlessWindow::show();
        if (window_ != nullptr) {
            glfwShowWindow(window_);
        }
    }

    void hide() override {
        HeadlessWindow::hide();
        if (window_ != nullptr) {
            glfwHideWindow(window_);
        }
    }

    void request_close() override {
        if (window_ != nullptr) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
        HeadlessWindow::request_close();
    }

    void pump_events() override {
        glfwPollEvents();

        if (window_ != nullptr && glfwWindowShouldClose(window_) == GLFW_TRUE) {
            if (!HeadlessWindow::close_requested()) {
                HeadlessWindow::request_close();
            }
            glfwSetWindowShouldClose(window_, GLFW_FALSE);
        }

        HeadlessWindow::pump_events();
    }

    [[nodiscard]] std::unique_ptr<SwapchainSurface> create_swapchain_surface(
        const SwapchainSurfaceRequest& request) override {
        if (request.hook) {
            if (auto surface = request.hook(request, window_)) {
                return surface;
            }
        }

        return std::make_unique<HeadlessSwapchainSurface>(
            request.renderer_backend,
            std::string{backend_name()},
            window_,
            request.user_data);
    }

private:
    void create_window() {
        const auto& cfg = config();

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, cfg.visible ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, cfg.resizable ? GLFW_TRUE : GLFW_FALSE);

        const auto width = static_cast<int>(std::min<std::uint32_t>(
            cfg.width, static_cast<std::uint32_t>(std::numeric_limits<int>::max())));
        const auto height = static_cast<int>(std::min<std::uint32_t>(
            cfg.height, static_cast<std::uint32_t>(std::numeric_limits<int>::max())));

        window_ = glfwCreateWindow(width, height, cfg.title.c_str(), nullptr, nullptr);
        if (window_ == nullptr) {
            const std::string message = GlfwLibrary::instance().last_error();
            throw std::runtime_error{
                message.empty() ? "Failed to create GLFW window"
                                 : "Failed to create GLFW window: " + message};
        }

        glfwSetWindowUserPointer(window_, this);
        install_callbacks();

        if (cfg.visible) {
            glfwShowWindow(window_);
        }
    }

    void install_callbacks() {
        glfwSetWindowCloseCallback(window_, [](GLFWwindow* window) {
            if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window))) {
                self->handle_close_request();
            }
        });

        glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height) {
            if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window))) {
                self->handle_resize(width, height);
            }
        });

        glfwSetWindowFocusCallback(window_, [](GLFWwindow* window, int focused) {
            if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window))) {
                self->handle_focus_change(focused == GLFW_TRUE);
            }
        });
    }

    void handle_close_request() {
        HeadlessWindow::request_close();
        if (window_ != nullptr) {
            glfwSetWindowShouldClose(window_, GLFW_FALSE);
        }
    }

    void handle_resize(int width, int height) {
        const auto clamped_width = width < 0 ? 0u : static_cast<std::uint32_t>(width);
        const auto clamped_height = height < 0 ? 0u : static_cast<std::uint32_t>(height);
        HeadlessWindow::post_event(Event::resized(clamped_width, clamped_height));
    }

    void handle_focus_change(bool focused) {
        HeadlessWindow::post_event(Event::focus_changed(focused));
    }

    GLFWwindow* window_{nullptr};
};

}  // namespace

std::shared_ptr<Window> create_glfw_window(WindowConfig config,
                                           std::shared_ptr<EventQueue> queue) {
    auto glfw_window = std::make_shared<GlfwWindow>(std::move(config), std::move(queue));
    return glfw_window;
}

}  // namespace engine::platform::windowing
