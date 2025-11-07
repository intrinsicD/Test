#include "engine/runtime/application.hpp"

#if ENGINE_ENABLE_RENDERING
#include "engine/rendering/backend/mock/presentation_backend.hpp"
#endif

#include <gtest/gtest.h>

namespace
{
    using Application = engine::runtime::Application;
    using ApplicationConfig = engine::runtime::ApplicationConfig;

    class RecordingApplication final : public Application
    {
    public:
        RecordingApplication(int& render_count, int& present_count
#if ENGINE_ENABLE_RENDERING
                              , bool& submit_function_available
#endif
        )
            : Application(make_config(present_count
#if ENGINE_ENABLE_RENDERING
                                      , submit_function_available
#endif
                ))
            , render_count_{render_count}
        {
        }

    protected:
        void on_initialize() override
        {
            frame_counter_ = 0;
        }

        void on_update(double) override
        {
            if (frame_counter_++ > 0)
            {
                quit();
            }
        }

        void on_render() override
        {
            auto& context = render_context();
            (void)context;
            ++render_count_;
        }

    private:
        static ApplicationConfig make_config(int& present_count
#if ENGINE_ENABLE_RENDERING
                                             , bool& submit_function_available
#endif
        )
        {
            ApplicationConfig config{};
            config.window = {
                .title = "Application Rendering Test",
                .width = 320,
                .height = 200,
                .visible = false,
                .resizable = false,
            };
            config.window_backend = engine::platform::WindowBackend::Mock;
            config.target_fps = 0.0;
#if ENGINE_ENABLE_RENDERING
            config.rendering.enable = true;
            config.rendering.backend = ApplicationConfig::RenderingConfig::Backend::Mock;
            auto backend = std::make_shared<engine::rendering::backend::mock::MockPresentationBackend>(
                [&present_count,
                 &submit_function_available](const engine::rendering::RuntimePresentationContext& context)
                {
                    ++present_count;
                    submit_function_available = context.submit_render_graph != nullptr;
                });
            config.rendering.backend_factory = [backend]() { return backend; };
#endif
            return config;
        }

        int& render_count_;
        int frame_counter_{0};
    };
}

TEST(ApplicationRendering, ProvidesContextAndInvokesPresentation)
{
    int render_count = 0;
    int present_count = 0;
#if ENGINE_ENABLE_RENDERING
    bool submit_function_available = false;
#endif

    RecordingApplication app{render_count, present_count
#if ENGINE_ENABLE_RENDERING
        , submit_function_available
#endif
    };
    const int exit_code = app.run();

    EXPECT_EQ(exit_code, 0);
    EXPECT_GE(render_count, 1);
#if ENGINE_ENABLE_RENDERING
    EXPECT_GE(present_count, 1);
    EXPECT_TRUE(submit_function_available);
#else
    EXPECT_EQ(present_count, 0);
#endif
}
