#include <gtest/gtest.h>

#include "engine/runtime/presentation_surface.hpp"

namespace
{
    using engine::runtime::RuntimePresentationSurfaceConfig;
}

TEST(RuntimePresentationSurface, CreatesMockSurfaceByDefault)
{
    RuntimePresentationSurfaceConfig config{};
    config.window.visible = false;
    config.window.resizable = true;
    config.window_backend = engine::platform::WindowBackend::Mock;
    config.renderer_backend = "mock";

    auto result = engine::runtime::create_presentation_surface(config);
    ASSERT_TRUE(result);

    auto surface = std::move(result).value();
    EXPECT_TRUE(surface.ready());
    ASSERT_NE(surface.window, nullptr);
    EXPECT_EQ(surface.window->backend_name(), std::string_view{"mock"});
    ASSERT_NE(surface.surface, nullptr);
    EXPECT_EQ(surface.surface->renderer_backend(), std::string_view{"mock"});
}

namespace
{
    class RecordingSurface final : public engine::platform::SwapchainSurface
    {
    public:
        explicit RecordingSurface(void* native, void* user)
            : native_surface_{native}
            , user_data_{user}
        {
        }

        [[nodiscard]] std::string_view renderer_backend() const noexcept override
        {
            return "custom";
        }

        [[nodiscard]] std::string_view window_backend() const noexcept override
        {
            return "mock";
        }

        [[nodiscard]] void* native_surface() const noexcept override
        {
            return native_surface_;
        }

        [[nodiscard]] void* user_data() const noexcept override
        {
            return user_data_;
        }

    private:
        void* native_surface_;
        void* user_data_;
    };
}

TEST(RuntimePresentationSurface, SurfaceHookOverridesImplementation)
{
    RuntimePresentationSurfaceConfig config{};
    config.window_backend = engine::platform::WindowBackend::Mock;
    config.renderer_backend = "test";
    int user_value = 42;
    bool hook_called = false;
    config.surface_user_data = &user_value;
    config.surface_hook = [&hook_called](
                                          const engine::platform::SwapchainSurfaceRequest& request,
                                          void* native_handle) -> std::unique_ptr<engine::platform::SwapchainSurface>
    {
        hook_called = true;
        EXPECT_EQ(request.renderer_backend, std::string{"test"});
        EXPECT_NE(native_handle, nullptr);
        return std::make_unique<RecordingSurface>(native_handle, request.user_data);
    };

    auto result = engine::runtime::create_presentation_surface(config);
    ASSERT_TRUE(result);

    auto surface = std::move(result).value();
    EXPECT_TRUE(hook_called);
    ASSERT_NE(surface.surface, nullptr);
    EXPECT_EQ(surface.surface->renderer_backend(), std::string_view{"custom"});
    EXPECT_EQ(surface.surface->user_data(), &user_value);
}

TEST(RuntimePresentationSurface, ReportsErrorWhenHookFails)
{
    RuntimePresentationSurfaceConfig config{};
    config.window_backend = engine::platform::WindowBackend::Mock;
    config.surface_hook = [](const engine::platform::SwapchainSurfaceRequest&, void*)
        -> std::unique_ptr<engine::platform::SwapchainSurface>
    {
        return nullptr;
    };

    auto result = engine::runtime::create_presentation_surface(config);
    ASSERT_FALSE(result);
    const auto error = result.error();
    EXPECT_EQ(error.identifier(), std::string{"engine.runtime.presentation_surface_creation_failed"});
}
