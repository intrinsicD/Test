#include "engine/runtime/presentation_surface.hpp"

#include <exception>
#include <utility>

namespace engine::runtime
{
    RuntimeResult<RuntimePresentationSurface>
    create_presentation_surface(const RuntimePresentationSurfaceConfig& config) noexcept
    {
        RuntimePresentationSurface surface{};
        platform::WindowConfig window_config = config.window;
        platform::WindowBackend backend = config.window_backend;

        std::shared_ptr<platform::EventQueue> queue = config.event_queue;
        if (!queue)
        {
            try
            {
                queue = platform::create_event_queue();
            }
            catch (const std::exception& exception)
            {
                return RuntimeResult<RuntimePresentationSurface>{
                    make_runtime_error(
                        RuntimeError::presentation_surface_creation_failed,
                        exception.what())};
            }
            catch (...)
            {
                return RuntimeResult<RuntimePresentationSurface>{
                    make_runtime_error(
                        RuntimeError::presentation_surface_creation_failed,
                        "Failed to create presentation event queue")};
            }
        }

        try
        {
            auto window = platform::create_window(
                window_config,
                backend,
                queue);
            if (!window)
            {
                return RuntimeResult<RuntimePresentationSurface>{
                    make_runtime_error(
                        RuntimeError::presentation_surface_backend_unavailable,
                        "Platform returned null window for presentation configuration")};
            }

            platform::SwapchainSurfaceRequest request{};
            request.renderer_backend = config.renderer_backend;
            request.hook = config.surface_hook;
            request.user_data = config.surface_user_data;

            auto swapchain_surface = window->create_swapchain_surface(request);
            if (!swapchain_surface)
            {
                return RuntimeResult<RuntimePresentationSurface>{
                    make_runtime_error(
                        RuntimeError::presentation_surface_creation_failed,
                        "Platform failed to create swapchain surface")};
            }

            surface.window = std::move(window);
            surface.event_queue = std::move(queue);
            surface.surface = std::move(swapchain_surface);
            return RuntimeResult<RuntimePresentationSurface>{std::move(surface)};
        }
        catch (const std::exception& exception)
        {
            return RuntimeResult<RuntimePresentationSurface>{
                make_runtime_error(
                    RuntimeError::presentation_surface_creation_failed,
                    exception.what())};
        }
        catch (...)
        {
            return RuntimeResult<RuntimePresentationSurface>{
                make_runtime_error(
                    RuntimeError::presentation_surface_creation_failed,
                    "Unexpected error while constructing presentation surface")};
        }
    }
}
