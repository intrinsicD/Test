#include <gtest/gtest.h>

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

#include "engine/platform/api.hpp"
#include "engine/platform/window.hpp"

namespace {

class HookedSurface final : public engine::platform::SwapchainSurface {
public:
    HookedSurface(std::string renderer_backend,
                  std::string window_backend,
                  void* native_surface,
                  void* user_data)
        : renderer_backend_(std::move(renderer_backend)),
          window_backend_(std::move(window_backend)),
          native_surface_(native_surface),
          user_data_(user_data) {}

    [[nodiscard]] std::string_view renderer_backend() const noexcept override {
        return renderer_backend_;
    }

    [[nodiscard]] std::string_view window_backend() const noexcept override {
        return window_backend_;
    }

    [[nodiscard]] void* native_surface() const noexcept override {
        return native_surface_;
    }

    [[nodiscard]] void* user_data() const noexcept override {
        return user_data_;
    }

private:
    std::string renderer_backend_;
    std::string window_backend_;
    void* native_surface_;
    void* user_data_;
};

}  // namespace

TEST(PlatformModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::platform::module_name(), "platform");
    EXPECT_STREQ(engine_platform_module_name(), "platform");
}

TEST(PlatformWindowing, MockWindowLifecycle) {
    using namespace engine::platform;

    WindowConfig config;
    config.title = "Unit Test";
    config.visible = false;

    auto window = create_window(config, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);
    EXPECT_EQ(window->backend_name(), "mock");
    EXPECT_FALSE(window->is_visible());

    window->show();
    EXPECT_TRUE(window->is_visible());
    window->hide();
    EXPECT_FALSE(window->is_visible());

    EXPECT_FALSE(window->close_requested());
    window->request_close();
    EXPECT_TRUE(window->close_requested());

    window->pump_events();
    EXPECT_FALSE(window->close_requested());

    Event event;
    ASSERT_TRUE(window->event_queue().poll(event));
    EXPECT_TRUE(event.type == EventType::CloseRequested);
    EXPECT_TRUE(window->event_queue().empty());
}

TEST(PlatformWindowing, EventDispatchFlow) {
    using namespace engine::platform;

    auto window = create_window(WindowConfig{}, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);

    window->post_event(Event::custom("payload"));
    window->post_event(Event::resized(640, 480));
    window->post_event(Event::focus_changed(true));

    window->pump_events();

    Event event;
    ASSERT_TRUE(window->event_queue().poll(event));
    EXPECT_TRUE(event.type == EventType::Custom);
    EXPECT_EQ(std::get<std::string>(event.payload), "payload");

    ASSERT_TRUE(window->event_queue().poll(event));
    EXPECT_TRUE(event.type == EventType::Resized);
    auto resize = std::get<ResizeEvent>(event.payload);
    EXPECT_EQ(resize.width, 640u);
    EXPECT_EQ(resize.height, 480u);

    ASSERT_TRUE(window->event_queue().poll(event));
    EXPECT_TRUE(event.type == EventType::FocusChanged);
    auto focus = std::get<FocusEvent>(event.payload);
    EXPECT_TRUE(focus.focused);

    EXPECT_FALSE(window->event_queue().poll(event));
}

TEST(PlatformWindowing, SwapchainHookIsInvoked) {
    using namespace engine::platform;

    auto window = create_window(WindowConfig{}, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);

    bool hook_called = false;
    void* hook_native = nullptr;

    SwapchainSurfaceRequest request{};
    request.renderer_backend = "test-renderer";
    request.user_data = reinterpret_cast<void*>(0x1234);
    request.hook = [&](const SwapchainSurfaceRequest& req, void* native_handle) {
        hook_called = true;
        hook_native = native_handle;
        return std::make_unique<HookedSurface>(
            std::string{req.renderer_backend},
            std::string{window->backend_name()},
            native_handle,
            req.user_data);
    };

    auto surface = window->create_swapchain_surface(request);
    ASSERT_TRUE(hook_called);
    ASSERT_TRUE(surface != nullptr);
    EXPECT_EQ(surface->renderer_backend(), "test-renderer");
    EXPECT_EQ(surface->window_backend(), window->backend_name());
    EXPECT_EQ(surface->native_surface(), hook_native);
    EXPECT_EQ(surface->user_data(), request.user_data);
}

TEST(PlatformWindowing, SwapchainFallbackWhenHookFails) {
    using namespace engine::platform;

    auto window = create_window(WindowConfig{}, WindowBackend::Mock);
    ASSERT_TRUE(window != nullptr);

    SwapchainSurfaceRequest request{};
    request.renderer_backend = "fallback";
    request.hook = [](const SwapchainSurfaceRequest&, void*) -> std::unique_ptr<SwapchainSurface> {
        return nullptr;
    };

    auto surface = window->create_swapchain_surface(request);
    ASSERT_TRUE(surface != nullptr);
    EXPECT_EQ(surface->renderer_backend(), "fallback");
    EXPECT_EQ(surface->window_backend(), window->backend_name());
    EXPECT_TRUE(surface->native_surface() != nullptr);
    EXPECT_EQ(surface->user_data(), request.user_data);
}

TEST(PlatformWindowing, GlfwBackendLifecycle) {
    using namespace engine::platform;

    WindowConfig config;
    config.title = "GLFW Backend Test";
    config.visible = false;

    std::shared_ptr<Window> window;
    try {
        window = create_window(config, WindowBackend::GLFW);
    } catch (const std::exception& error) {
#if defined(GTEST_SKIP)
        GTEST_SKIP() << "GLFW backend unavailable: " << error.what();
#else
        std::cout << "[  SKIPPED ] GLFW backend unavailable: " << error.what() << '\n';
        return;
#endif
    }

    ASSERT_TRUE(window != nullptr);
    EXPECT_EQ(window->backend_name(), "glfw");

    EXPECT_FALSE(window->close_requested());
    window->request_close();
    window->pump_events();

    Event event;
    ASSERT_TRUE(window->event_queue().poll(event));
    EXPECT_TRUE(event.type == EventType::CloseRequested);
}

