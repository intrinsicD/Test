#include "window_base.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace engine::platform::windowing
{
    namespace
    {
        class GlfwLibrary
        {
        public:
            static GlfwLibrary& instance()
            {
                static GlfwLibrary library;
                return library;
            }

            void retain()
            {
                std::unique_lock lock{mutex_};
                wait_for_initialisation(lock);

                if (ref_count_ > 0)
                {
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
                if (result != GLFW_TRUE)
                {
                    const std::string message = last_error_;
                    ready_.notify_all();
                    lock.unlock();
                    init_failed(message);
                }

                ref_count_ = 1;
                ready_.notify_all();
                lock.unlock();
            }

            void release() noexcept
            {
                std::unique_lock lock{mutex_};
                if (ref_count_ == 0)
                {
                    return;
                }

                --ref_count_;
                if (ref_count_ == 0)
                {
                    glfwTerminate();
                    glfwSetErrorCallback(nullptr);
                    lock.unlock();
                }
            }

            void record_error(int code, const char* description) noexcept
            {
                std::scoped_lock lock{mutex_};
                std::ostringstream builder;
                builder << "GLFW error " << code;
                if (description != nullptr && description[0] != '\0')
                {
                    builder << ": " << description;
                }
                last_error_ = builder.str();
            }

            [[nodiscard]] std::string last_error() const
            {
                std::scoped_lock lock{mutex_};
                return last_error_;
            }

        private:
            GlfwLibrary() = default;

            static void handle_error(int code, const char* description) noexcept
            {
                instance().record_error(code, description);
            }

            void wait_for_initialisation(std::unique_lock<std::mutex>& lock)
            {
                ready_.wait(lock, [this] { return !initialising_; });
            }

            [[noreturn]] void init_failed(const std::string& message)
            {
                glfwSetErrorCallback(nullptr);
                if (message.empty())
                {
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

        class GlfwWindow final : public HeadlessWindow
        {
        public:
            GlfwWindow(WindowConfig config, std::shared_ptr<EventQueue> queue)
                : HeadlessWindow("glfw", std::move(config), std::move(queue))
            {
                headless_ = this->config().requires_headless_safe();

                auto& library = GlfwLibrary::instance();
                library.retain();

                try
                {
                    create_window();
                }
                catch (...)
                {
                    library.release();
                    throw;
                }
            }

            ~GlfwWindow() noexcept override
            {
                if (window_ != nullptr)
                {
                    glfwDestroyWindow(window_);
                    window_ = nullptr;
                }

                GlfwLibrary::instance().release();
            }

            void show() override
            {
                if (headless_)
                {
                    return;
                }

                HeadlessWindow::show();
                if (window_ != nullptr)
                {
                    glfwShowWindow(window_);
                }
            }

            void hide() override
            {
                HeadlessWindow::hide();
                if (!headless_ && window_ != nullptr)
                {
                    glfwHideWindow(window_);
                }
            }

            void request_close() override
            {
                if (window_ != nullptr)
                {
                    glfwSetWindowShouldClose(window_, GLFW_TRUE);
                }
                HeadlessWindow::request_close();
            }

            void pump_events() override
            {
                glfwPollEvents();

                if (window_ != nullptr && glfwWindowShouldClose(window_) == GLFW_TRUE)
                {
                    if (!HeadlessWindow::close_requested())
                    {
                        HeadlessWindow::request_close();
                    }
                    glfwSetWindowShouldClose(window_, GLFW_FALSE);
                }

                HeadlessWindow::pump_events();
            }

            [[nodiscard]] std::unique_ptr<SwapchainSurface> create_swapchain_surface(
                const SwapchainSurfaceRequest& request) override
            {
                if (request.hook)
                {
                    if (auto surface = request.hook(request, window_))
                    {
                        return surface;
                    }
                }

                return std::make_unique<HeadlessSwapchainSurface>(
                    request.renderer_backend,
                    std::string{backend_name()},
                    window_,
                    request.user_data);
            }

            [[nodiscard]] void* native_handle() noexcept override
            {
                return window_;
            }

        private:
            void create_window()
            {
                const auto& cfg = config();

                const int visibility_hint = headless_ ? GLFW_FALSE : (cfg.visible ? GLFW_TRUE : GLFW_FALSE);

                glfwDefaultWindowHints();
                // Configure OpenGL 4.6 Core Profile for rendering backend
                glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_VISIBLE, visibility_hint);
                glfwWindowHint(GLFW_RESIZABLE, cfg.resizable ? GLFW_TRUE : GLFW_FALSE);

                const auto width = static_cast<int>(std::min<std::uint32_t>(
                    cfg.width, static_cast<std::uint32_t>(std::numeric_limits<int>::max())));
                const auto height = static_cast<int>(std::min<std::uint32_t>(
                    cfg.height, static_cast<std::uint32_t>(std::numeric_limits<int>::max())));

                window_ = glfwCreateWindow(width, height, cfg.title.c_str(), nullptr, nullptr);
                if (window_ == nullptr)
                {
                    const std::string message = GlfwLibrary::instance().last_error();
                    throw std::runtime_error{
                        message.empty()
                            ? "Failed to create GLFW window"
                            : "Failed to create GLFW window: " + message
                    };
                }

                glfwSetWindowUserPointer(window_, this);
                install_callbacks();

                if (cfg.visible && !headless_)
                {
                    glfwShowWindow(window_);
                }
                else
                {
                    HeadlessWindow::hide();
                }
            }

            void install_callbacks()
            {
                glfwSetWindowCloseCallback(window_, [](GLFWwindow* window)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_close_request();
                    }
                });

                glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_resize(width, height);
                    }
                });

                glfwSetWindowFocusCallback(window_, [](GLFWwindow* window, int focused)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_focus_change(focused == GLFW_TRUE);
                    }
                });

                // Input callbacks
                glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_key_event(key, action);
                    }
                });

                glfwSetMouseButtonCallback(window_, [](GLFWwindow* window, int button, int action, int /*mods*/)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_mouse_button_event(button, action);
                    }
                });

                glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xpos, double ypos)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_cursor_position_event(xpos, ypos);
                    }
                });

                glfwSetScrollCallback(window_, [](GLFWwindow* window, double xoffset, double yoffset)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_scroll_event(xoffset, yoffset);
                    }
                });

                glfwSetDropCallback(window_, [](GLFWwindow* window, int count, const char** paths)
                {
                    if (auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window)))
                    {
                        self->handle_file_drop(count, paths);
                    }
                });
            }

            void handle_close_request()
            {
                HeadlessWindow::request_close();
                if (window_ != nullptr)
                {
                    glfwSetWindowShouldClose(window_, GLFW_FALSE);
                }
            }

            void handle_resize(int width, int height)
            {
                const auto clamped_width = width < 0 ? 0u : static_cast<std::uint32_t>(width);
                const auto clamped_height = height < 0 ? 0u : static_cast<std::uint32_t>(height);
                HeadlessWindow::post_event(Event::resized(clamped_width, clamped_height));
            }

            void handle_focus_change(bool focused)
            {
                HeadlessWindow::post_event(Event::focus_changed(focused));
            }

            void handle_key_event(int key, int action)
            {
                const input::Key mapped_key = map_glfw_key(key);
                const bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
                input_state().apply_key_event(mapped_key, pressed);
            }

            void handle_mouse_button_event(int button, int action)
            {
                const input::MouseButton mapped_button = map_glfw_mouse_button(button);
                const bool pressed = (action == GLFW_PRESS);
                input_state().apply_mouse_button_event(mapped_button, pressed);
            }

            void handle_cursor_position_event(double xpos, double ypos)
            {
                input_state().apply_cursor_position(static_cast<float>(xpos), static_cast<float>(ypos));
            }

            void handle_scroll_event(double xoffset, double yoffset)
            {
                input_state().apply_scroll_delta(static_cast<float>(xoffset), static_cast<float>(yoffset));
            }

            void handle_file_drop(int count, const char** paths)
            {
                if (count <= 0 || paths == nullptr)
                {
                    return;
                }

                std::vector<std::filesystem::path> dropped;
                dropped.reserve(static_cast<std::size_t>(count));
                for (int index = 0; index < count; ++index)
                {
                    if (paths[index] == nullptr)
                    {
                        continue;
                    }

                    try
                    {
                        dropped.emplace_back(std::filesystem::path{paths[index]});
                    }
                    catch (const std::exception&)
                    {
                        // Ignore malformed paths provided by the backend.
                    }
                }

                if (!dropped.empty())
                {
                    HeadlessWindow::post_event(Event::file_drop(std::move(dropped)));
                }
            }

            static input::Key map_glfw_key(int glfw_key)
            {
                switch (glfw_key)
                {
                    case GLFW_KEY_ESCAPE: return input::Key::Escape;
                    case GLFW_KEY_SPACE: return input::Key::Space;
                    case GLFW_KEY_ENTER: return input::Key::Enter;
                    case GLFW_KEY_TAB: return input::Key::Tab;
                    case GLFW_KEY_BACKSPACE: return input::Key::Backspace;
                    case GLFW_KEY_LEFT_SHIFT: return input::Key::LeftShift;
                    case GLFW_KEY_RIGHT_SHIFT: return input::Key::RightShift;
                    case GLFW_KEY_LEFT_CONTROL: return input::Key::LeftCtrl;
                    case GLFW_KEY_RIGHT_CONTROL: return input::Key::RightCtrl;
                    case GLFW_KEY_LEFT_ALT: return input::Key::LeftAlt;
                    case GLFW_KEY_RIGHT_ALT: return input::Key::RightAlt;
                    case GLFW_KEY_LEFT_SUPER: return input::Key::LeftSuper;
                    case GLFW_KEY_RIGHT_SUPER: return input::Key::RightSuper;
                    case GLFW_KEY_UP: return input::Key::Up;
                    case GLFW_KEY_DOWN: return input::Key::Down;
                    case GLFW_KEY_LEFT: return input::Key::Left;
                    case GLFW_KEY_RIGHT: return input::Key::Right;
                    case GLFW_KEY_W: return input::Key::W;
                    case GLFW_KEY_A: return input::Key::A;
                    case GLFW_KEY_S: return input::Key::S;
                    case GLFW_KEY_D: return input::Key::D;
                    case GLFW_KEY_Q: return input::Key::Q;
                    case GLFW_KEY_E: return input::Key::E;
                    case GLFW_KEY_0: return input::Key::Digit0;
                    case GLFW_KEY_1: return input::Key::Digit1;
                    case GLFW_KEY_2: return input::Key::Digit2;
                    case GLFW_KEY_3: return input::Key::Digit3;
                    case GLFW_KEY_4: return input::Key::Digit4;
                    case GLFW_KEY_5: return input::Key::Digit5;
                    case GLFW_KEY_6: return input::Key::Digit6;
                    case GLFW_KEY_7: return input::Key::Digit7;
                    case GLFW_KEY_8: return input::Key::Digit8;
                    case GLFW_KEY_9: return input::Key::Digit9;
                    default: return input::Key::Unknown;
                }
            }

            static input::MouseButton map_glfw_mouse_button(int glfw_button)
            {
                switch (glfw_button)
                {
                    case GLFW_MOUSE_BUTTON_LEFT: return input::MouseButton::Left;
                    case GLFW_MOUSE_BUTTON_RIGHT: return input::MouseButton::Right;
                    case GLFW_MOUSE_BUTTON_MIDDLE: return input::MouseButton::Middle;
                    case GLFW_MOUSE_BUTTON_4: return input::MouseButton::Extra1;
                    case GLFW_MOUSE_BUTTON_5: return input::MouseButton::Extra2;
                    default: return input::MouseButton::Left;
                }
            }

            GLFWwindow* window_{nullptr};
            bool headless_{false};
        };
    } // namespace

    std::shared_ptr<Window> create_glfw_window(WindowConfig config,
                                               std::shared_ptr<EventQueue> queue)
    {
        auto glfw_window = std::make_shared<GlfwWindow>(std::move(config), std::move(queue));
        return glfw_window;
    }
} // namespace engine::platform::windowing
