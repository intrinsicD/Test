#pragma once

#include "engine/runtime/errors.hpp"

#include "engine/platform/windowing/window.hpp"

#include <memory>
#include <string>

namespace engine::runtime
{
    /**
     * \brief Shared configuration for constructing presentation surfaces.
     *
     * The configuration bundles the window description, backend selection, and
     * optional swapchain hook so runtime hosts, tooling harnesses, and
     * presentation backends can materialise consistent surfaces without
     * duplicating setup code.
     */
    struct RuntimePresentationSurfaceConfig
    {
        /// Human-readable window configuration propagated to the platform
        /// module. Defaults to a hidden, resizable window so headless and
        /// interactive paths share a single code path.
        platform::WindowConfig window{
            .title = "Runtime Presentation",
            .width = 1280,
            .height = 720,
            .visible = false,
            .resizable = true,
        };

        /// Preferred window backend. Use `Auto` to delegate to the platform's
        /// selection logic or request `Mock`/`GLFW` explicitly when the caller
        /// needs deterministic behaviour.
        platform::WindowBackend window_backend{platform::WindowBackend::Auto};

        /// Identifier describing the renderer that will consume the surface.
        /// Propagated to `platform::SwapchainSurfaceRequest::renderer_backend`
        /// so tooling can diagnose mismatches.
        std::string renderer_backend{"mock"};

        /// Optional callback allowing render backends to create specialised
        /// swapchain surfaces. When unset the platform layer falls back to the
        /// generic `HeadlessSwapchainSurface` implementation.
        platform::SwapchainSurfaceRequest::Hook surface_hook{};

        /// Optional opaque pointer forwarded to the created surface.
        void* surface_user_data{nullptr};

        /// Optional event queue shared with the window implementation. When
        /// null a queue is allocated automatically.
        std::shared_ptr<platform::EventQueue> event_queue{};
    };

    /**
     * \brief Aggregates the window, event queue, and swapchain surface created
     *        from a \ref RuntimePresentationSurfaceConfig.
     */
    struct RuntimePresentationSurface
    {
        std::shared_ptr<platform::Window> window{};
        std::shared_ptr<platform::EventQueue> event_queue{};
        std::unique_ptr<platform::SwapchainSurface> surface{};

        [[nodiscard]] bool has_window() const noexcept { return window != nullptr; }
        [[nodiscard]] bool has_surface() const noexcept { return surface != nullptr; }
        [[nodiscard]] bool ready() const noexcept { return has_window() && has_surface(); }

        void release_surface() noexcept { surface.reset(); }

        void reset() noexcept
        {
            surface.reset();
            window.reset();
            event_queue.reset();
        }
    };

    /**
     * \brief Construct a presentation surface using the supplied configuration.
     *
     * Errors are reported using \ref RuntimeError::presentation_surface_* codes
     * so callers can surface consistent diagnostics alongside the runtime
     * stage planner telemetry.
     */
    RuntimeResult<RuntimePresentationSurface>
    create_presentation_surface(const RuntimePresentationSurfaceConfig& config) noexcept;
}
